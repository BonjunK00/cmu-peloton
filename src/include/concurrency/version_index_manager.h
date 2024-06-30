#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>

#include "common/item_pointer.h"
#include "common/internal_types.h"
#include "common/synchronization/spin_latch.h"
#include "concurrency/transaction_context.h"

namespace peloton {
namespace concurrency {

class VersionIndexManager {
  public:
    static VersionIndexManager *GetInstance(void);

    void AddVersionEntry(ItemPointer* indirection_ptr, 
                        const ItemPointer &old_location,
                        const ItemPointer &new_location);
                        
    ItemPointer GetVisibleVersion(ItemPointer* indirection_ptr,
                                  TransactionContext *const current_txn,
                                  const VisibilityIdType type = VisibilityIdType::READ_ID);

  private:
    VersionIndexManager();
    ~VersionIndexManager();

    std::unordered_map<ItemPointer *, std::vector<ItemPointer>> version_index_;

    common::synchronization::SpinLatch version_index_lock;

};
}
}