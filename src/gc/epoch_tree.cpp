#include "gc/epoch_tree.h"

namespace peloton {
namespace gc {

EpochLeafNode* EpochTree::InsertEpochNode(const eid_t &epoch) {
  EpochLeafNode* new_leaf = new EpochLeafNode(epoch);

  if(root == nullptr) {
    root = new_leaf;
    right_most_leaf = new_leaf;
    return new_leaf;
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
    return new_leaf;
  }
  
  dynamic_cast<EpochInternalNode*>(current->parent)->right = new_internal;
  new_internal->parent = current->parent;
  new_internal->left = current;
  current->parent = new_internal;

  current = new_internal;
  while(current->parent != nullptr) {
    EpochInternalNode* parent = dynamic_cast<EpochInternalNode*>(current->parent);
    parent->size = parent->left->size + parent->right->size;
    parent->epoch_end = dynamic_cast<EpochInternalNode*>(parent->right)->epoch_end;
    current = parent;
  }

  right_most_leaf = new_leaf;
  return new_leaf;
}

EpochLeafNode* EpochTree::FindLeafNode(const eid_t &epoch) {
  EpochNode* current = root;

  while (current != nullptr) {
    if (current->IsLeaf()) {
      auto leaf = dynamic_cast<EpochLeafNode*>(current);
      if (leaf != nullptr && leaf->epoch == epoch) {
        return leaf;
      }
      break;
    }

    auto internal = dynamic_cast<EpochInternalNode*>(current);
    if (internal == nullptr) {
      break;
    }

    if (internal->left != nullptr) {
      if (internal->left->IsLeaf()) {
        auto left_leaf = dynamic_cast<EpochLeafNode*>(internal->left);
        if (left_leaf != nullptr && epoch == left_leaf->epoch) {
          return left_leaf;
        }
      } else {
        auto left_internal = dynamic_cast<EpochInternalNode*>(internal->left);
        if (left_internal != nullptr && epoch <= left_internal->epoch_end) {
          current = internal->left;
          continue;
        }
      }
    }

    current = internal->right;
  }

  return nullptr;
}

void EpochTree::DeleteEpochNode(EpochNode* node) {
  if(node == root) {
    root = nullptr;
    right_most_leaf = nullptr;
    delete node;
    return;
  }

  EpochNode* parent = node->parent;
  EpochInternalNode* internal = dynamic_cast<EpochInternalNode*>(parent);

  if(internal->left == node) {
    internal->left = nullptr;
  } else {
    internal->right = nullptr;
  }

  delete node;
}

void EpochTree::PrintEpochTree() {
  std::cout << "Epoch Tree:" << std::endl;
  PrintEpochNode(root);
}

void EpochTree::PrintEpochNode(gc::EpochNode* node) {
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
    PrintEpochNode(internal->left);
    PrintEpochNode(internal->right);
  }
}
  
} // namespace gc
} // namespace peloton
