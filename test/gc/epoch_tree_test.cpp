#include <iostream>

#include "common/harness.h"
#include "gc/epoch_tree.h"

namespace peloton {
namespace test {

//===--------------------------------------------------------------------===//
// Garbage Collection Tests
//===--------------------------------------------------------------------===//

class EpochTreeTests : public PelotonTest {};

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
  
  epoch_tree.PrintEpochTree();

  DeleteTree(&epoch_tree, epoch_tree.root);
}

}  // namespace test
}  // namespace peloton
