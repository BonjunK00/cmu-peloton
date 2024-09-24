#include <iostream>

#include "common/harness.h"
#include "gc/epoch_tree.h"

namespace peloton {
namespace test {

//===--------------------------------------------------------------------===//
// Garbage Collection Tests
//===--------------------------------------------------------------------===//

class EpochTreeTests : public PelotonTest {};

void TraverseAndPrint(gc::EpochNode* node) {
  if (node == nullptr) return;

  if (node->IsLeaf()) {
    auto leaf = dynamic_cast<gc::EpochLeafNode*>(node);
    if (leaf) {
      std::cout << "Leaf Node - Epoch: " << leaf->epoch 
                << ", Ref Count: " << leaf->ref_count 
                << ", Size: " << leaf->size << std::endl;
    }
  } else {
    auto internal = dynamic_cast<gc::EpochInternalNode*>(node);
    if (internal) {
      std::cout << "Internal Node - Epoch Range: [" << internal->epoch_start 
                << ", " << internal->epoch_end << "]" 
                << ", Size: " << internal->size <<  std::endl;
    }
  }

  if (!node->IsLeaf()) {
    auto internal = dynamic_cast<gc::EpochInternalNode*>(node);
    TraverseAndPrint(internal->left);
    TraverseAndPrint(internal->right);
  }
}

void DeleteTree(gc::EpochTree* epoch_tree, gc::EpochNode* node) {
  if (node == nullptr) return;

  if (!node->IsLeaf()) {
    auto internal = dynamic_cast<gc::EpochInternalNode*>(node);
    DeleteTree(epoch_tree, internal->left);
    DeleteTree(epoch_tree, internal->right);
  }

  epoch_tree->DeleteEpochNode(node);
}

TEST_F(EpochTreeTests, InsertTest) {
  gc::EpochTree epoch_tree;
  epoch_tree.InsertEpochNode(1);
  epoch_tree.InsertEpochNode(2);
  epoch_tree.InsertEpochNode(3);
  epoch_tree.InsertEpochNode(4);
  epoch_tree.InsertEpochNode(5);
  epoch_tree.InsertEpochNode(6);
  epoch_tree.InsertEpochNode(7);
  epoch_tree.InsertEpochNode(8);
  epoch_tree.InsertEpochNode(9);
  epoch_tree.InsertEpochNode(10);

  EXPECT_EQ(epoch_tree.right_most_leaf->epoch, 10);
  EXPECT_EQ(epoch_tree.right_most_leaf->ref_count, 0);
  EXPECT_EQ(epoch_tree.right_most_leaf->txns.size(), 0);
  
  TraverseAndPrint(epoch_tree.root);

  DeleteTree(&epoch_tree, epoch_tree.root);
}

}  // namespace test
}  // namespace peloton
