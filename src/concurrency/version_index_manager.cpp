#include "concurrency/version_index_manager.h"

#include "concurrency/transaction_manager_factory.h"
#include "storage/storage_manager.h"
#include "storage/tile_group_header.h"

namespace peloton {
namespace concurrency {

VersionIndexManager::VersionIndexManager() = default;

VersionIndexManager::~VersionIndexManager() = default;

VersionIndexManager *VersionIndexManager::GetInstance(void) {
  static VersionIndexManager version_index_manager;
  return &version_index_manager;
}

void VersionIndexManager::AddVersionEntry(ItemPointer* indirection_ptr, 
    const ItemPointer &old_location, const ItemPointer &new_location) {
  version_index_lock.Lock();

  auto& entries = version_index_[indirection_ptr];

  if (old_location.IsNull()) {
    entries.push_back(new_location);
  } else {
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
      if (*it == old_location) {
        entries.insert(it.base(), new_location);
        break;
      }
    }
  }

  version_index_lock.Unlock();
}

ItemPointer VersionIndexManager::GetVisibleVersion(ItemPointer* indirection_ptr, 
    TransactionContext *const current_txn, const VisibilityIdType type) {
  version_index_lock.Lock();
  
  ItemPointer result;
  
  auto it = version_index_.find(indirection_ptr);
  if (it != version_index_.end()) {
    cid_t txn_vis_id;

    if (type == VisibilityIdType::READ_ID) {
      txn_vis_id = current_txn->GetReadId();
    } else {
      PELOTON_ASSERT(type == VisibilityIdType::COMMIT_ID);
      txn_vis_id = current_txn->GetCommitId();
    }

    const auto& entries = it->second;
    auto entry_it = std::lower_bound(entries.begin(), entries.end(), txn_vis_id,
                                      [this, current_txn](const ItemPointer& entry, cid_t txn_vis_id) {
                                        auto tile_group_header = storage::StorageManager::GetInstance()->GetTileGroup(entry.block)->GetHeader();
                                        oid_t tuple_id = entry.offset;

                                        cid_t tuple_end_cid = tile_group_header->GetEndCommitId(tuple_id);
                                        return tuple_end_cid <= txn_vis_id;
                                      });

    if (entry_it != entries.end()) {
      auto tile_group_header = storage::StorageManager::GetInstance()->GetTileGroup(entry_it->block)->GetHeader();
      auto &transaction_manager = TransactionManagerFactory::GetInstance();
      if (transaction_manager.IsVisible(current_txn, tile_group_header, entry_it->offset) == VisibilityType::OK) {
        result = *entry_it;
      }
    }
  }
  
  version_index_lock.Unlock();
  
  return result;
}

}
}