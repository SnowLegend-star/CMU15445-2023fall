//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "common/config.h"
#include "common/rid.h"
#include "concurrency/transaction.h"
#include "execution/executors/delete_executor.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value.h"
#include "concurrency/transaction_manager.h"
#include "execution/execution_common.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  child_executor_->Init();
  delete_finish_ = false;
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  if (delete_finish_) {
    return false;
  }
  delete_finish_ = true;
  TableInfo *table_info = GetExecutorContext()->GetCatalog()->GetTable(plan_->table_oid_);
  auto table_indexes = GetExecutorContext()->GetCatalog()->GetTableIndexes(table_info->name_);
  int cnt = 0;  // 对删除的行数进行计数
  Tuple old_tuple{};
  RID old_rid{};
  auto cur_txn=GetExecutorContext()->GetTransaction();
  auto txn_mgr=GetExecutorContext()->GetTransactionManager();
  auto tmp_ts=cur_txn->GetTransactionTempTs();

  while (child_executor_->Next(&old_tuple, &old_rid)) {
    auto base_tuple_meta=table_info->table_->GetTupleMeta(old_rid);
    auto base_tuple_ts=base_tuple_meta.ts_;
    // 存在写冲突直接返回    
    // 不能删除已经被删除的tuple
    if((base_tuple_ts>=TXN_START_ID&&base_tuple_ts!=tmp_ts)|| base_tuple_meta.is_deleted_){ 
      cur_txn->SetTainted();
      throw ExecutionException("Delete存在type1型写冲突");
    }
    
    // 还有另一种写写冲突 当前tuple已经存在了一个更新的版本，就不能继续修改当前版本了
    if(base_tuple_ts<TXN_START_ID&&base_tuple_ts>cur_txn->GetReadTs()){
      cur_txn->SetTainted();
      throw ExecutionException("Delete存在type2型写冲突");
    }

    cnt++;
    table_info->table_->UpdateTupleMeta(TupleMeta{tmp_ts, true}, old_rid); // 昏头了，proj4这里不能简单将ts设置为0了
    cur_txn->AppendWriteSet(table_info->oid_, old_rid); // 把当前rid加入写集

    for (auto &index : table_indexes) {
      auto old_key = old_tuple.KeyFromTuple(table_info->schema_, index->key_schema_, index->index_->GetKeyAttrs());
      index->index_->DeleteEntry(old_key, old_rid, GetExecutorContext()->GetTransaction());
    }


    auto col_cnt=child_executor_->GetOutputSchema().GetColumnCount();
    std::vector<bool> modified_field(col_cnt,true); // delete相当于修改了当前tuple的所哟attrs
    auto prev_link_opt=txn_mgr->GetUndoLink(old_rid);
    // 只有当前不是处于insert状态时，才可以正常更新版本链
    if(!cur_txn->GetInsertState()){
      if(prev_link_opt.has_value()){
        UndoLink prev_link=prev_link_opt.value();
        // 如果发现当前本来就处于修改(update/Del)的状态，也不需要生成新的undolog
        if(base_tuple_ts>=TXN_START_ID&&base_tuple_ts==tmp_ts){
          auto prev_link=prev_link_opt.value();
          auto cur_undolog=cur_txn->GetUndoLog(prev_link.prev_log_idx_);
          cur_undolog.is_deleted_=true;
          cur_undolog.modified_fields_=modified_field;
          cur_undolog.ts_=tmp_ts; // 这个改不改都行
          cur_undolog.tuple_=old_tuple;
          cur_txn->ModifyUndoLog(prev_link.prev_log_idx_,cur_undolog);              
          
        }else{
          auto new_link=cur_txn->AppendUndoLog({true,modified_field,old_tuple,tmp_ts,prev_link});
          txn_mgr->UpdateUndoLink(old_rid, new_link);            
        }
      }else{
        // 如果发现当前处于insert的状态, 则不需要更新版本链
        auto new_link=cur_txn->AppendUndoLog({true,modified_field,old_tuple,tmp_ts,{}});
        txn_mgr->UpdateUndoLink(old_rid, new_link);     
      }
    }


  }
  // TxnMgrDbg("Deletet后的结果", txn_mgr, table_info, table_info->table_.get());
  std::vector<Value> res = {{TypeId::INTEGER, cnt}};
  *tuple = Tuple{res, &GetOutputSchema()};
  return true;
}

}  // namespace bustub
