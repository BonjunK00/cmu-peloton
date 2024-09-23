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
                << ", Ref Count: " << leaf->ref_count << std::endl;
    }
  } else {
    auto internal = dynamic_cast<gc::EpochInternalNode*>(node);
    if (internal) {
      std::cout << "Internal Node - Epoch Range: [" << internal->epoch_start 
                << ", " << internal->epoch_end << "]" << std::endl;
    }
  }

  if (!node->IsLeaf()) {
    auto internal = dynamic_cast<gc::EpochInternalNode*>(node);
    TraverseAndPrint(internal->left);
    TraverseAndPrint(internal->right);
  }
}

TEST_F(EpochTreeTests, InsertTest) {
  gc::EpochTree epoch_tree;
  epoch_tree.InsertEpochNode(1);
  epoch_tree.InsertEpochNode(2);
  epoch_tree.InsertEpochNode(3);

  TraverseAndPrint(epoch_tree.root);
  
  EXPECT_EQ(epoch_tree.right_most_leaf->epoch, 3);
  EXPECT_EQ(epoch_tree.right_most_leaf->ref_count, 0);
  EXPECT_EQ(epoch_tree.right_most_leaf->txns.size(), 0);
}

}  // namespace test
}  // namespace peloton
