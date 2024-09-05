#include "gc/epoch_tree_node.h"

namespace peloton {
namespace gc {

EpochTreeLeafNode* EpochTreeInternalNode::FindLeafNode(eid_t epoch) {
  if (IsLeaf()) return nullptr;

  if (epoch <= this->epoch && left) {
    if (left->IsLeaf()) {
        auto leftLeaf = static_cast<EpochTreeLeafNode*>(left.get());
        if (leftLeaf->epoch == epoch) {
          return leftLeaf;
        }
    } else {
      return static_cast<EpochTreeInternalNode*>(left.get())->FindLeafNode(epoch);
    }
  }

  if (epoch > this->epoch && right) {
    if (right->IsLeaf()) {
      auto rightLeaf = static_cast<EpochTreeLeafNode*>(right.get());
      if (rightLeaf->epoch == epoch) {
        return rightLeaf; 
      }
    } else {
      return static_cast<EpochTreeInternalNode*>(right.get())->FindLeafNode(epoch);
    }
  }

  return nullptr;
}

void EpochTreeInternalNode::InsertLeafNode(std::shared_ptr<EpochTreeLeafNode> node,
		std::shared_ptr<EpochTreeNode> parent_node) {
  if (node->epoch <= this->epoch) {
    if (!left) {
      left = node;
			node->parent = parent_node;
    } else if (left->IsLeaf()) {
      auto existingLeaf = std::static_pointer_cast<EpochTreeLeafNode>(left);
      auto newInternal = std::make_shared<EpochTreeInternalNode>(
        std::min(existingLeaf->epoch, node->epoch));

      if (existingLeaf->epoch <= node->epoch) {
        newInternal->left = existingLeaf;
        newInternal->right = node;
      } else {
          newInternal->left = node;
          newInternal->right = existingLeaf;
      }

			existingLeaf->parent = newInternal;
    	node->parent = newInternal;
			newInternal->parent = parent_node;
      left = newInternal;
    } else {
        std::static_pointer_cast<EpochTreeInternalNode>(left)->InsertLeafNode(node, left);
    }
  } else if (node->epoch > this->epoch) {
    if (!right) {
      right = node;
			node->parent = parent_node;
    } else if (right->IsLeaf()) {
      auto existingLeaf = std::static_pointer_cast<EpochTreeLeafNode>(right);
      auto newInternal = std::make_shared<EpochTreeInternalNode>(existingLeaf->epoch);

      if (existingLeaf->epoch < node->epoch) {
          newInternal->left = existingLeaf;
          newInternal->right = node;
      } else {
          newInternal->left = node;
          newInternal->right = existingLeaf;
      }

			existingLeaf->parent = newInternal;
			node->parent = newInternal;
			newInternal->parent = parent_node;
      right = newInternal;
    } else {
      std::static_pointer_cast<EpochTreeInternalNode>(right)->InsertLeafNode(node, right);
    }
  }
}

void EpochTreeInternalNode::DeleteLeafNode(eid_t epoch) {
	if (epoch <= this->epoch && left) {
		if (left->IsLeaf()) {
			auto leftLeaf = static_cast<EpochTreeLeafNode*>(left.get());
			if (leftLeaf->epoch == epoch) {
				left.reset();
			}
		} else {
			static_cast<EpochTreeInternalNode*>(left.get())->DeleteLeafNode(epoch);
			if (!static_cast<EpochTreeInternalNode*>(left.get())->left &&
					!static_cast<EpochTreeInternalNode*>(left.get())->right) {
				left.reset();
			}
		}
	} else if (epoch > this->epoch && right) {
		if (right->IsLeaf()) {
			auto rightLeaf = static_cast<EpochTreeLeafNode*>(right.get());
			if (rightLeaf->epoch == epoch) {
				right.reset();
			}
		} else {
			static_cast<EpochTreeInternalNode*>(right.get())->DeleteLeafNode(epoch);

			if (!static_cast<EpochTreeInternalNode*>(right.get())->left &&
					!static_cast<EpochTreeInternalNode*>(right.get())->right) {
				right.reset();
			}
		}
	}
}

}
}