#pragma once

#include "common/internal_types.h"
#include "concurrency/transaction_context.h"

namespace peloton {
namespace gc {

class EpochNode {
  public:
    EpochNode* parent;
    eid_t size;

    EpochNode(eid_t size) : parent(nullptr), size(size) {}
    virtual ~EpochNode() = default;
    virtual bool IsLeaf() const = 0;
};

class EpochInternalNode : public EpochNode {
 public:
  EpochNode* left;
  EpochNode* right;
  eid_t epoch_start;
  eid_t epoch_end;

  EpochInternalNode(eid_t start, eid_t end) : 
      left(nullptr), right(nullptr), 
      epoch_start(start), epoch_end(end), EpochNode(end - start + 1) {}
  
  bool IsLeaf() const override {
    return false;
  }
};

class EpochLeafNode : public EpochNode {
 public:
  eid_t epoch;
  int ref_count;
  std::vector<concurrency::TransactionContext* > txns;

  EpochLeafNode(eid_t epoch) : epoch(epoch), ref_count(0), EpochNode(1) {}
  
  bool IsLeaf() const override {
    return true;
  }
};

class EpochTree {
 public:
  EpochNode* root;
  EpochLeafNode* right_most_leaf;

  EpochTree() : root(nullptr), right_most_leaf(nullptr) {}

  void InsertEpochNode(eid_t epoch);

};

}
}