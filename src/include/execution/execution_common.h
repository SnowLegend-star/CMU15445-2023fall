#pragma once

#include <string>
#include <vector>

#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"
#include "storage/table/tuple.h"

namespace bustub {

auto ReconstructTuple(const Schema *schema, const Tuple &base_tuple, const TupleMeta &base_meta,
                      const std::vector<UndoLog> &undo_logs) -> std::optional<Tuple>;

void TxnMgrDbg(const std::string &info, TransactionManager *txn_mgr, const TableInfo *table_info,
               TableHeap *table_heap);


// 根据需要添加新函数...
// 你可能需要定义一些额外的函数。
//
// 为了让你了解在执行器 / 事务管理器之间可以共享哪些函数，下面是我们在参考实现中定义的
// 辅助函数名称列表。你应该在实现过程中根据需要提出自己的函数。
// * CollectUndoLogs
// * WalkUndoLogs
// * Modify
// * IsWriteWriteConflict
// * GenerateDiffLog
// * GenerateNullTupleForSchema
// * GetUndoLogSchema
//
// 我们没有提供这些函数的签名，因为它们取决于你对系统其他部分的实现。
// 你不需要在自己的实现中定义相同的辅助函数。请根据需要添加你自己的函数，以避免到处重复编写相同的代码。

// 获得undoLog的schema
auto GetUndoLogSchema(const Schema *base_schema,const std::vector<bool>& modified_fields) ->Schema;

// 检测是否可能存在写冲突
auto IsWriteWriteConflict() ->bool;

// 得到这个update的上一个已提交tuple
auto GetModifiedField(TransactionManager txn_mgr,Transaction txn,UndoLog &cur_undolog,Schema &schema)->std::vector<bool>;
}  // namespace bustub
