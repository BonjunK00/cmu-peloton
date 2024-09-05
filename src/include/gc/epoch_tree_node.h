#pragma once

#include <thread>
#include <vector>

#include "common/internal_types.h"
#include "concurrency/transaction_context.h"

namespace peloton {
namespace gc {

class EpochTreeNode {
public:
  virtual ~EpochTreeNode() = default;
  virtual bool IsLeaf() const = 0;
};

class EpochTreeLeafNode : public EpochTreeNode {
public:
  int ref_count;
  eid_t epoch;
  std::shared_ptr<EpochTreeNode> parent;
  std::vector<concurrency::TransactionContext* > transactions;

  EpochTreeLeafNode(eid_t epoch) : ref_count(0), epoch(epoch), parent(nullptr) {}

  void IncrementRefCount() {
    ref_count++;
  }

  void DecrementRefCount() {
    if (ref_count > 0) ref_count--;
  }

  bool IsGarbage() const {
    return ref_count == 0;
  }

  bool IsLeaf() const override {
    return true;
  }
};

class EpochTreeInternalNode : public EpochTreeNode {
public:
  eid_t epoch;
  bool has_zero_ref_count;
  std::shared_ptr<EpochTreeNode> left;
  std::shared_ptr<EpochTreeNode> right;
  std::shared_ptr<EpochTreeNode> parent;

  EpochTreeInternalNode(eid_t epoch) : epoch(epoch), has_zero_ref_count(false), 
    left(nullptr), right(nullptr), parent(nullptr) {}

  EpochTreeLeafNode* FindLeafNode(eid_t epoch);
  void InsertLeafNode(std::shared_ptr<EpochTreeLeafNode> node,
      std::shared_ptr<EpochTreeNode> parent_node);
  void DeleteLeafNode(eid_t epoch);

  bool IsLeaf() const override {
    return false;
  }
};

}
}
