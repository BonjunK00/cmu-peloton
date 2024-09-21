#pragma once

namespace peloton {
namespace gc {

class EpochTreeNode {
  public:
    virtual ~EpochTreeNode() = default;
};

class EpochTreeInternalNode : public EpochTreeNode {
 public:
  EpochTreeNode* left;
  EpochTreeNode* right;
  EpochTreeInternalNode* parent;
  int epoch_start;
  int epoch_end;

  EpochTreeInternalNode(int start, int end) : 
      left(nullptr), right(nullptr), parent(nullptr),
      epoch_start(start), epoch_end(end) {}
};

class EpochTreeLeafNode : public EpochTreeNode {
 public:
  int epoch;
  int ref_count;
  EpochTreeLeafNode* parent;

  EpochTreeLeafNode(int epoch) : epoch(epoch), ref_count(0), parent(nullptr) {}
};

class EpochTree {
 public:
  EpochTreeNode* root;
  EpochTreeLeafNode* right_most_leaf;

  EpochTree() : root(nullptr), right_most_leaf(nullptr) {}
};

}
}