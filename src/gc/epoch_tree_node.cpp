#include "gc/epoch_tree_node.h"

namespace peloton {
namespace gc {

EpochTreeLeafNode* EpochTreeInternalNode::FindLeafNode(eid_t epoch) {
    if (IsLeaf()) return nullptr;
    if (epoch == this->epoch && left && left->IsLeaf()) return static_cast<EpochTreeLeafNode*>(left.get());
    if (epoch < this->epoch && left) return static_cast<EpochTreeInternalNode*>(left.get())->FindLeafNode(epoch);
    if (epoch > this->epoch && right) return static_cast<EpochTreeInternalNode*>(right.get())->FindLeafNode(epoch);
    return nullptr;
}

void EpochTreeInternalNode::InsertLeafNode(std::shared_ptr<EpochTreeLeafNode> node) {
    if (node->epoch < this->epoch) {
        if (!left) left = node;
        else if (left->IsLeaf()) left = std::make_shared<EpochTreeInternalNode>(node->epoch);
        else static_cast<EpochTreeInternalNode*>(left.get())->InsertLeafNode(node);
    } else if (node->epoch > this->epoch) {
        if (!right) right = node;
        else if (right->IsLeaf()) right = std::make_shared<EpochTreeInternalNode>(node->epoch);
        else static_cast<EpochTreeInternalNode*>(right.get())->InsertLeafNode(node);
    }
}

void EpochTreeInternalNode::DeleteLeafNode(eid_t epoch) {
    if (epoch < this->epoch && left) {
        if (left->IsLeaf() && static_cast<EpochTreeLeafNode*>(left.get())->epoch == epoch) {
            left.reset();
        } else {
            static_cast<EpochTreeInternalNode*>(left.get())->DeleteLeafNode(epoch);
        }
    } else if (epoch > this->epoch && right) {
        if (right->IsLeaf() && static_cast<EpochTreeLeafNode*>(right.get())->epoch == epoch) {
            right.reset();
        } else {
            static_cast<EpochTreeInternalNode*>(right.get())->DeleteLeafNode(epoch);
        }
    }
}

}
}