#pragma once

#include <thread>
#include <vector>

#include "common/internal_types.h"

namespace peloton {
namespace gc {

class EpochTreeNode {
public:
    virtual ~EpochTreeNode() = default;
    virtual bool IsLeaf() const = 0;
};

class EpochTreeLeafNode : public EpochTreeNode {
public:
    EpochTreeLeafNode(eid_t epoch) : ref_count(0), epoch(epoch) {}

    int ref_count;
    eid_t epoch;
    std::vector<txn_id_t> transactions;

    EpochTreeLeafNode(int epoch) : ref_count(0), epoch(epoch) {}

    void IncrementRefCount() {
        ref_count++;
    }

    void DecrementRefCount() {
        if (ref_count > 0) ref_count--;
    }

    bool IsGarbage() const {
        return ref_count == 0;
    }

    bool IsLeaf() const override {
        return true;
    }
};

class EpochTreeInternalNode : public EpochTreeNode {
public:
    eid_t epoch;
    std::shared_ptr<EpochTreeNode> left;
    std::shared_ptr<EpochTreeNode> right;

    EpochTreeInternalNode(eid_t epoch) : epoch(epoch), left(nullptr), right(nullptr) {}

    EpochTreeLeafNode* FindLeafNode(eid_t epoch);
    void InsertLeafNode(std::shared_ptr<EpochTreeLeafNode> EpochTreeNode);
    void DeleteLeafNode(eid_t epoch);

    bool IsLeaf() const override {
        return false;
    }
};

}
}
