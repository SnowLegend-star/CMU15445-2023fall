//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "common/rid.h"
#include "execution/executor_context.h"
#include "execution/executors/insert_executor.h"
#include "storage/table/table_heap.h"
#include "storage/table/tuple.h"
#include "type/type.h"
#include "type/type_id.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"
#include "execution/execution_common.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  plan_ = plan;
  child_executor_ = std::move(child_executor);
}

void InsertExecutor::Init() {
  child_executor_->Init();
  insert_finish_ = false;
}

auto InsertExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 防止重复插入
  if (insert_finish_) {
    return false;
  }
  
  insert_finish_ = true;
  TableInfo *table_info = GetExecutorContext()->GetCatalog()->GetTable(plan_->GetTableOid());
  auto schema = table_info->schema_;
  auto table_indexes = GetExecutorContext()->GetCatalog()->GetTableIndexes(table_info->name_);
  int cnt = 0;  // 待插入的tuple数
  while (child_executor_->Next(tuple, rid)) {
    auto cur_txn=GetExecutorContext()->GetTransaction();
    cur_txn->UpdateInsertState();
    // auto txn_mgr=GetExecutorContext()->GetTransactionManager();
    auto tmp_ts=cur_txn->GetTransactionTempTs();
    cnt++;
    std::optional<RID> new_rid_optional = table_info->table_->InsertTuple(TupleMeta{tmp_ts, false}, *tuple);
      
    // 没有多余的空间进行插入了直接返回false
    if(!new_rid_optional.has_value()){    
      return false;
    }
    RID new_rid = new_rid_optional.value();    
    cur_txn->AppendWriteSet(table_info->oid_, new_rid);   // 将rid写入当前txn的写集, 留给后面commit
    // 更新索引
    for (auto index_info : table_indexes) {
      auto key = tuple->KeyFromTuple(schema, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      index_info->index_->InsertEntry(key, new_rid, exec_ctx_->GetTransaction());  // 可以更新索引吗
    }
    // 更新当前tuple的版本链   Insert不需要更新版本链
    // std::vector<bool> modified_fields(GetOutputSchema().GetColumnCount(),true); // 插入是更新所有attrs
    // auto prev_link=cur_txn->AppendUndoLog({false,modified_fields,*tuple,tmp_ts,{}}); // insert会生成一个全新的版本链
    // txn_mgr->UpdateVersionLink(new_rid, VersionUndoLink{prev_link},nullptr);
  }
  // 构造输出table
  std::vector<Value> res = {{TypeId::INTEGER, cnt}};
  *tuple = Tuple(res, &GetOutputSchema());
  return true;
}

}  // namespace bustub
