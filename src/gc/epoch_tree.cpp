#include "gc/epoch_tree.h"

namespace peloton {
namespace gc {

void EpochTree::InsertEpochNode(const eid_t &epoch) {
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

  current = new_internal;
  while(current->parent != nullptr) {
    EpochInternalNode* parent = dynamic_cast<EpochInternalNode*>(current->parent);
    parent->size = parent->left->size + parent->right->size;
    parent->epoch_end = dynamic_cast<EpochInternalNode*>(parent->right)->epoch_end;
    current = parent;
  }

  right_most_leaf = new_leaf;
}

EpochLeafNode* EpochTree::FindLeafNode(const eid_t &epoch) {
  EpochNode* current = root;

  while (current != nullptr && !current->IsLeaf()) {
    EpochInternalNode* internal = dynamic_cast<EpochInternalNode*>(current);

    if (internal->left->IsLeaf() 
        && epoch == dynamic_cast<EpochLeafNode*>(internal->left)->epoch) {
      current = internal->left;
      break;
    }

    if (internal->right->IsLeaf() 
        && epoch == dynamic_cast<EpochLeafNode*>(internal->right)->epoch) {
      current = internal->right;
      break;
    }

    if (epoch <= dynamic_cast<EpochInternalNode*>(internal->left)->epoch_end) {
      current = internal->left;
    } else {
      current = internal->right;
    }
  }

  if(current == nullptr || !current->IsLeaf()) {
    return nullptr;
  }

  return dynamic_cast<EpochLeafNode*>(current);
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
