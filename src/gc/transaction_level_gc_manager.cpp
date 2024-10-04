//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// transaction_level_gc_manager.cpp
//
// Identification: src/gc/transaction_level_gc_manager.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "gc/transaction_level_gc_manager.h"

#include "brain/query_logger.h"
#include "catalog/manager.h"
#include "common/container_tuple.h"
#include "concurrency/epoch_manager_factory.h"
#include "concurrency/transaction_manager_factory.h"
#include "index/index.h"
#include "settings/settings_manager.h"
#include "storage/database.h"
#include "storage/storage_manager.h"
#include "storage/tile_group.h"
#include "storage/tuple.h"
#include "threadpool/mono_queue_pool.h"

namespace peloton {
namespace gc {

bool TransactionLevelGCManager::ResetTuple(const ItemPointer &location) {
  auto storage_manager = storage::StorageManager::GetInstance();
  auto tile_group = storage_manager->GetTileGroup(location.block).get();

  auto tile_group_header = tile_group->GetHeader();

  // Reset the header
  tile_group_header->SetTransactionId(location.offset, INVALID_TXN_ID);
  tile_group_header->SetLastReaderCommitId(location.offset, INVALID_CID);
  tile_group_header->SetBeginCommitId(location.offset, MAX_CID);
  tile_group_header->SetEndCommitId(location.offset, MAX_CID);
  tile_group_header->SetNextItemPointer(location.offset, INVALID_ITEMPOINTER);
  tile_group_header->SetPrevItemPointer(location.offset, INVALID_ITEMPOINTER);
  tile_group_header->SetIndirection(location.offset, nullptr);

  // Reclaim the varlen pool
  CheckAndReclaimVarlenColumns(tile_group, location.offset);

  LOG_TRACE("Garbage tuple(%u, %u) is reset", location.block, location.offset);
  return true;
}

void TransactionLevelGCManager::Running(const int &thread_id) {
  PELOTON_ASSERT(is_running_ == true);
  uint32_t backoff_shifts = 0;
  while (true) {
    int reclaimed_count = Reclaim(thread_id);
    int unlinked_count = Unlink(thread_id);

    if (is_running_ == false) {
      return;
    }
    if (reclaimed_count == 0 && unlinked_count == 0) {
      // sleep at most 0.8192 s
      if (backoff_shifts < 13) {
        ++backoff_shifts;
      }
      uint64_t sleep_duration = 1UL << backoff_shifts;
      sleep_duration *= 100;
      std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration));
    } else {
      backoff_shifts >>= 1;
    }
  }
}

void TransactionLevelGCManager::RecycleTransaction(
    concurrency::TransactionContext *txn) {
  auto &epoch_manager = concurrency::EpochManagerFactory::GetInstance();

  epoch_manager.ExitEpoch(txn->GetThreadId(), txn->GetEpochId());

  if (!txn->IsReadOnly() && \
      txn->GetResult() != ResultType::SUCCESS && txn->IsGCSetEmpty() != true) {
    txn->SetEpochId(epoch_manager.GetNextEpochId());
  }

  // Add the transaction context to the lock-free queue
  unlink_queues_[HashToThread(txn->GetThreadId())]->Enqueue(txn);
}

int TransactionLevelGCManager::Unlink(const int &thread_id) {
  int tuple_counter = 0;

  // check if any garbage can be unlinked from indexes.
  // every time we garbage collect at most MAX_ATTEMPT_COUNT tuples.
  std::vector<concurrency::TransactionContext *> garbages;
  
  GarbageNode current_garbage;
  for (size_t i = 0; i < MAX_ATTEMPT_COUNT; ++i) {
    while(current_garbage.epoch_node == nullptr) {
      if(garbage_queue_.Dequeue(current_garbage) == false) {
        break;
      };
    }

    if(current_garbage.epoch_node == nullptr) {
      break;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - current_garbage.insertion_time);
    auto grace_period = std::chrono::milliseconds(GRACE_PERIOD);

    if (duration < grace_period) {
      continue;
    }

    if(epoch_insertion_time_map_.find(current_garbage.epoch_node) == epoch_insertion_time_map_.end()
        || current_garbage.insertion_time != epoch_insertion_time_map_[current_garbage.epoch_node]) {
      if(garbage_queue_.Dequeue(current_garbage) == false) {
        break;
      }
      continue;
    }

    if(current_garbage.epoch_node->IsLeaf()) {
      auto leaf = dynamic_cast<EpochLeafNode*>(current_garbage.epoch_node);
      
      if(leaf->ref_count > 0) {
        if(garbage_queue_.Dequeue(current_garbage) == false) {
          break;
        }
        continue;
      }

      while(leaf->txns.IsEmpty() == false) {
        concurrency::TransactionContext* txn = nullptr;
        leaf->txns.Dequeue(txn);
        
        UnlinkVersions(txn);
        garbages.push_back(txn);
        tuple_counter++;

        // Log the query into query_history_catalog
        if (settings::SettingsManager::GetBool(settings::SettingId::brain)) {
          std::vector<std::string> query_strings = txn->GetQueryStrings();
          if (query_strings.size() != 0) {
            uint64_t timestamp = txn->GetTimestamp();
            auto &pool = threadpool::MonoQueuePool::GetBrainInstance();
            for (auto query_string : query_strings) {
              pool.SubmitTask([query_string, timestamp] {
                brain::QueryLogger::LogQuery(query_string, timestamp);
              });
            }
          }
        }
      }
    }
    
    EpochInternalNode* parent = dynamic_cast<EpochInternalNode*>(current_garbage.epoch_node->parent);
    epoch_tree_.DeleteEpochNode(current_garbage.epoch_node);
    epoch_insertion_time_map_.erase(current_garbage.epoch_node);

    if(parent != nullptr && parent->left == nullptr && parent->right == nullptr) {
      GarbageNode new_garbage(parent);
      epoch_insertion_time_map_[parent] = now;
      garbage_queue_.Enqueue(new_garbage);
    }

    if(garbage_queue_.Dequeue(current_garbage) == false) {
      break;
    }
  }

  for (auto &item : garbages) {
    reclaim_maps_[thread_id].insert(item);
  }
  LOG_TRACE("Marked %d tuples as garbage", tuple_counter);
  return tuple_counter;
}

// executed by a single thread. so no synchronization is required.
int TransactionLevelGCManager::Reclaim(const int &thread_id) {
  int gc_counter = 0;

  // we delete garbage in the free list
  auto garbage_ctx_entry = reclaim_maps_[thread_id].begin();
  while (garbage_ctx_entry != reclaim_maps_[thread_id].end()) {
    AddToRecycleMap(*garbage_ctx_entry);

    // Remove from the original map
    garbage_ctx_entry = reclaim_maps_[thread_id].erase(garbage_ctx_entry);
    gc_counter++;
  }
  LOG_TRACE("Marked %d txn contexts as recycled", gc_counter);
  return gc_counter;
}

// Multiple GC thread share the same recycle map
void TransactionLevelGCManager::AddToRecycleMap(
    concurrency::TransactionContext *txn_ctx) {
  for (auto &entry : *(txn_ctx->GetGCSetPtr().get())) {
    auto storage_manager = storage::StorageManager::GetInstance();
    auto tile_group = storage_manager->GetTileGroup(entry.first);

    // During the resetting, a table may be deconstructed because of the DROP
    // TABLE request
    if (tile_group == nullptr) {
      delete txn_ctx;
      return;
    }

    PELOTON_ASSERT(tile_group != nullptr);

    storage::DataTable *table =
        dynamic_cast<storage::DataTable *>(tile_group->GetAbstractTable());
    PELOTON_ASSERT(table != nullptr);

    oid_t table_id = table->GetOid();
    auto tile_group_header = tile_group->GetHeader();
    PELOTON_ASSERT(tile_group_header != nullptr);
    bool immutable = tile_group_header->GetImmutability();

    for (auto &element : entry.second) {
      // as this transaction has been committed, we should reclaim older
      // versions.
      ItemPointer location(entry.first, element.first);

      // If the tuple being reset no longer exists, just skip it
      if (ResetTuple(location) == false) {
        continue;
      }
      // if immutable is false and the entry for table_id exists.
      if ((!immutable) &&
          recycle_queue_map_.find(table_id) != recycle_queue_map_.end()) {
        recycle_queue_map_[table_id]->Enqueue(location);
      }
    }
  }

  auto storage_manager = storage::StorageManager::GetInstance();
  for (auto &entry : *(txn_ctx->GetGCObjectSetPtr().get())) {
    oid_t database_oid = std::get<0>(entry);
    oid_t table_oid = std::get<1>(entry);
    oid_t index_oid = std::get<2>(entry);
    PELOTON_ASSERT(database_oid != INVALID_OID);
    auto database = storage_manager->GetDatabaseWithOid(database_oid);
    PELOTON_ASSERT(database != nullptr);
    if (table_oid == INVALID_OID) {
      storage_manager->RemoveDatabaseFromStorageManager(database_oid);
      continue;
    }
    auto table = database->GetTableWithOid(table_oid);
    PELOTON_ASSERT(table != nullptr);
    if (index_oid == INVALID_OID) {
      database->DropTableWithOid(table_oid);
      LOG_DEBUG("GCing table %u", table_oid);
      continue;
    }
    auto index = table->GetIndexWithOid(index_oid);
    PELOTON_ASSERT(index != nullptr);
    table->DropIndexWithOid(index_oid);
    LOG_DEBUG("GCing index %u", index_oid);
  }

  delete txn_ctx;
}

// this function returns a free tuple slot, if one exists
// called by data_table.
ItemPointer TransactionLevelGCManager::ReturnFreeSlot(const oid_t &table_id) {
  // for catalog tables, we directly return invalid item pointer.
  if (recycle_queue_map_.find(table_id) == recycle_queue_map_.end()) {
    return INVALID_ITEMPOINTER;
  }
  ItemPointer location;
  PELOTON_ASSERT(recycle_queue_map_.find(table_id) != recycle_queue_map_.end());
  auto recycle_queue = recycle_queue_map_[table_id];

  if (recycle_queue->Dequeue(location) == true) {
    LOG_TRACE("Reuse tuple(%u, %u) in table %u", location.block,
              location.offset, table_id);
    return location;
  }
  return INVALID_ITEMPOINTER;
}

void TransactionLevelGCManager::ClearGarbage(int thread_id) {
  while (!garbage_queue_.IsEmpty()) {
    Unlink(thread_id);
  }

  while (reclaim_maps_[thread_id].size() != 0) {
    Reclaim(thread_id);
  }

  return;
}

void TransactionLevelGCManager::StopGC() {
  LOG_TRACE("Stopping GC");
  this->is_running_ = false;
  // clear the garbage in each GC thread
  for (int thread_id = 0; thread_id < gc_thread_count_; ++thread_id) {
    ClearGarbage(thread_id);
  }
}

void TransactionLevelGCManager::UnlinkVersions(
    concurrency::TransactionContext *txn_ctx) {
  for (auto entry : *(txn_ctx->GetGCSetPtr().get())) {
    for (auto &element : entry.second) {
      UnlinkVersion(ItemPointer(entry.first, element.first), element.second);
    }
  }
}

// delete a tuple from all its indexes it belongs to.
void TransactionLevelGCManager::UnlinkVersion(const ItemPointer location,
                                              GCVersionType type) {
  // get indirection from the indirection array.
  auto tile_group =
      storage::StorageManager::GetInstance()->GetTileGroup(location.block);

  // if the corresponding tile group is deconstructed,
  // then do nothing.
  if (tile_group == nullptr) {
    return;
  }

  auto tile_group_header =
      storage::StorageManager::GetInstance()->GetTileGroup(location.block)->GetHeader();

  ItemPointer *indirection = tile_group_header->GetIndirection(location.offset);

  // do nothing if indirection is null
  if (indirection == nullptr) {
    return;
  }

  ContainerTuple<storage::TileGroup> current_tuple(tile_group.get(),
                                                   location.offset);

  storage::DataTable *table =
      dynamic_cast<storage::DataTable *>(tile_group->GetAbstractTable());
  PELOTON_ASSERT(table != nullptr);

  // NOTE: for now, we only consider unlinking tuple versions from primary
  // indexes.
  if (type == GCVersionType::COMMIT_UPDATE) {
    // the gc'd version is an old version.
    // this version needs to be reclaimed by the GC.
    // if the version differs from the previous one in some columns where
    // secondary indexes are built on, then we need to unlink the previous
    // version from the secondary index.
  } else if (type == GCVersionType::COMMIT_DELETE) {
    // the gc'd version is an old version.
    // need to recycle this version as well as its newer (empty) version.
    // we also need to delete the tuple from the primary and secondary
    // indexes.
  } else if (type == GCVersionType::ABORT_UPDATE) {
    // the gc'd version is a newly created version.
    // if the version differs from the previous one in some columns where
    // secondary indexes are built on, then we need to unlink this version
    // from the secondary index.

  } else if (type == GCVersionType::ABORT_DELETE) {
    // the gc'd version is a newly created empty version.
    // need to recycle this version.
    // no index manipulation needs to be made.
  } else {
    PELOTON_ASSERT(type == GCVersionType::ABORT_INSERT ||
                   type == GCVersionType::COMMIT_INS_DEL ||
                   type == GCVersionType::ABORT_INS_DEL);

    // attempt to unlink the version from all the indexes.
    for (size_t idx = 0; idx < table->GetIndexCount(); ++idx) {
      auto index = table->GetIndex(idx);
      if (index == nullptr) continue;
      auto index_schema = index->GetKeySchema();
      auto indexed_columns = index_schema->GetIndexedColumns();

      // build key.
      std::unique_ptr<storage::Tuple> current_key(
          new storage::Tuple(index_schema, true));
      current_key->SetFromTuple(&current_tuple, indexed_columns,
                                index->GetPool());

      index->DeleteEntry(current_key.get(), indirection);
    }
  }
}

void TransactionLevelGCManager::InsertEpochNode(const eid_t &epoch_id) {
  EpochLeafNode *epoch_node = epoch_tree_.InsertEpochNode(epoch_id);
  if (epoch_node == nullptr) {
    return;
  }
  auto now = std::chrono::steady_clock::now();
  epoch_insertion_time_map_[epoch_node] = now;
  garbage_queue_.Enqueue(GarbageNode(epoch_node, now));
}

void TransactionLevelGCManager::IncrementEpochNodeRefCount(const eid_t &epoch_id) {
  EpochLeafNode *epoch_node = epoch_tree_.FindLeafNode(epoch_id);
  if (epoch_node == nullptr) {
    return;
  }
  epoch_node->ref_count++;
}

void TransactionLevelGCManager::DecrementEpochNodeRefCount(const eid_t &epoch_id) {
  EpochLeafNode *epoch_node = epoch_tree_.FindLeafNode(epoch_id);
  if (epoch_node == nullptr) {
    return;
  }
  epoch_node->ref_count--;
  if (epoch_node->ref_count <= 0) {
    auto now = std::chrono::steady_clock::now();
    epoch_insertion_time_map_[epoch_node] = now;
    garbage_queue_.Enqueue(GarbageNode(epoch_node, now));
  }
}

void TransactionLevelGCManager::BindEpochNode(const eid_t &epoch_id, concurrency::TransactionContext *txn) {
  EpochLeafNode *epoch_node = epoch_tree_.FindLeafNode(epoch_id);
  if (epoch_node == nullptr) {
    return;
  }
  epoch_node->txns.Enqueue(txn);
}

}  // namespace gc
}  // namespace peloton
