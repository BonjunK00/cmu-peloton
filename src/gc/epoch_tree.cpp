#include "gc/epoch_tree.h"

namespace peloton {
namespace gc {

void EpochTree::InsertEpochNode(eid_t epoch) {
  EpochLeafNode* new_leaf = new EpochLeafNode(epoch);

  if(root == nullptr) {
    root = new_leaf;
    right_most_leaf = new_leaf;
    return;
  }

  EpochNode* current = right_most_leaf;

  while(current->parent != nullptr) {
    EpochInternalNode* parent = dynamic_cast<EpochInternalNode*>(current->parent);
    
    if(parent->left == nullptr) {
      continue;
    }
    
    if(parent->right == nullptr || parent->right->size < parent->left->size) {
      break;
    }

    current = parent;
  }

  eid_t start = current->IsLeaf() 
    ? dynamic_cast<EpochLeafNode*>(current)->epoch 
    : dynamic_cast<EpochInternalNode*>(current)->epoch_start;

  EpochInternalNode* new_internal = new EpochInternalNode(start, epoch);
  new_internal->right = new_leaf;
  new_leaf->parent = new_internal;

  if(current == root) {
    root = new_internal;
    new_internal->left = current;
    current->parent = new_internal;
    right_most_leaf = new_leaf;
    return;
  }
  
  dynamic_cast<EpochInternalNode*>(current->parent)->right = new_internal;
  new_internal->parent = current->parent;
  new_internal->left = current;
  current->parent = new_internal;

  right_most_leaf = new_leaf;
}

}
}