//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "catalog/catalog.h"
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "common/exception.h"
#include "common/rid.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"
#include "execution/execution_common.h"
#include "execution/executors/update_executor.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/plans/update_plan.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  // As of Fall 2022, you DON'T need to implement update executor to have perfect score in project 3 / project 4.
  plan_ = plan;
  child_executor_ = std::move(child_executor);
}

void UpdateExecutor::Init() {
  child_executor_->Init();
  update_finish_ = false;
}

// 查询计划没有filter_predicate_, 说明update是作用于所有tuple的
// 先将旧的tuple删除再插入更新的tuple 这是proj 3的做法
// proj4应该是直接使用UpdateTupleInPlace
auto UpdateExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // update只执行一次
  if (update_finish_) {
    return false;
  }
  update_finish_ = true;

  table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->table_oid_);
  // auto table_heap=table_info_->table_.get();
  auto table_indexes = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
  int cnt = 0;
  Tuple old_tuple{};
  RID old_rid{};
  auto cur_txn = GetExecutorContext()->GetTransaction();
  auto txn_mgr = GetExecutorContext()->GetTransactionManager();
  auto tmp_ts = cur_txn->GetTransactionTempTs();

  while (child_executor_->Next(&old_tuple, &old_rid)) {
    auto base_tuple_ts = table_info_->table_->GetTupleMeta(old_rid).ts_;
    // 存在写冲突直接返回
    if (base_tuple_ts >= TXN_START_ID && base_tuple_ts != tmp_ts) {
      cur_txn->SetTainted();
      throw ExecutionException("Update存在type1型写冲突");
    }

    // 还有另一种写写冲突 当前tuple已经存在了一个更新的版本，就不能继续修改当前版本了
    if(base_tuple_ts<TXN_START_ID&&base_tuple_ts>cur_txn->GetReadTs()){
      cur_txn->SetTainted();
      throw ExecutionException("Update存在type2型写冲突");
    }
    cnt++;
    // 构造新的tuple
    auto col_cnt = child_executor_->GetOutputSchema().GetColumnCount();
    std::vector<Value> new_value{};
    std::vector<bool> modified_field(col_cnt, false);
    new_value.reserve(plan_->target_expressions_.size());  // 其实这个new_value和原value差不多
    for (const auto &attr : plan_->target_expressions_) {  // 把new_value填充为完整的value
      new_value.push_back(attr->Evaluate(&old_tuple, child_executor_->GetOutputSchema()));
    }
    Tuple new_tuple = Tuple{new_value, &table_info_->schema_};
    Tuple update_tuple = new_tuple;  // 用value构造完整的tuple

    // 就地更新元组
    table_info_->table_->UpdateTupleInPlace(TupleMeta{tmp_ts, false}, update_tuple, old_rid);

    cur_txn->AppendWriteSet(table_info_->oid_, old_rid);  // 把更新的元素加入写集

    for (auto &index_info : table_indexes) {
      auto index = index_info->index_.get();
      auto key_attrs = index_info->index_->GetKeyAttrs();
      auto old_key = old_tuple.KeyFromTuple(table_info_->schema_, *index->GetKeySchema(), key_attrs);
      auto new_key = update_tuple.KeyFromTuple(table_info_->schema_, *index->GetKeySchema(), key_attrs);
      // // 删除旧的索引
      // index->DeleteEntry(old_key, old_rid, exec_ctx_->GetTransaction());
      // 插入新的索引  是不是不用插入新的索引了
      // index->InsertEntry(new_key, new_rid, exec_ctx_->GetTransaction());
    }

    // 生成update操作修改的parital_tuple


    std::vector<Value> values;  // 存储更改的部分
    std::vector<Column> cols;
    
    for (uint32_t i = 0; i < col_cnt; i++) {
      // 利用col的value进行判断是否更改
      auto old_value = old_tuple.GetValue(&child_executor_->GetOutputSchema(), i);
      auto new_value = new_tuple.GetValue(&child_executor_->GetOutputSchema(), i);
      if (!old_value.CompareExactlyEquals(new_value)) {
        modified_field[i] = true;
        values.emplace_back(new_value);
        cols.emplace_back(child_executor_->GetOutputSchema().GetColumn(i));
      }
    }
    Schema partial_schema = Schema{cols};
    auto parital_tuple = Tuple{values, &partial_schema};

    // 如果发现当前处于insert的状态, 则不需要更新版本链
    // 只有当前不是处于insert状态时，才可以正常更新版本链
    if (!cur_txn->GetInsertState()) {
      auto prev_link_opt = txn_mgr->GetUndoLink(old_rid);
      if (prev_link_opt.has_value()) {
        auto prev_link = prev_link_opt.value();

        // 如果发现当前本来就处于修改(update/Del)的状态，也不需要生成新的undolog
        if (base_tuple_ts >= TXN_START_ID && base_tuple_ts == tmp_ts) {
          auto prev_link = prev_link_opt.value();
          auto cur_undolog = cur_txn->GetUndoLog(prev_link.prev_log_idx_);

          // 注意，这里的modified_field还要参考之前的
          auto txn_id=cur_undolog.prev_version_.prev_txn_;
          auto prev_undolog_txn=txn_mgr->txn_map_[txn_id];
          auto prev_undolog_idx=cur_undolog.prev_version_.prev_log_idx_;
          auto base_tuple=prev_undolog_txn->GetUndoLog(prev_undolog_idx).tuple_;
          values.clear();
          cols.clear();
          for(uint32_t i=0;i<col_cnt;i++){
            modified_field[i]=false;
          }
          for (uint32_t i = 0; i < col_cnt; i++) {
            // 利用col的value进行判断是否更改
            auto base_value = base_tuple.GetValue(&child_executor_->GetOutputSchema(), i);
            auto new_value = new_tuple.GetValue(&child_executor_->GetOutputSchema(), i);
            if (!base_value.CompareExactlyEquals(new_value)) {
              modified_field[i] = true;
              values.emplace_back(new_value);
              cols.emplace_back(child_executor_->GetOutputSchema().GetColumn(i));
            }
          }
          auto partial_schema = Schema{cols};
          auto parital_tuple = Tuple{values, &partial_schema};

          cur_undolog.is_deleted_ = false;
          cur_undolog.modified_fields_ = modified_field;
          cur_undolog.ts_ = tmp_ts;  // 这个改不改都行
          cur_undolog.tuple_ = parital_tuple;
          cur_txn->ModifyUndoLog(prev_link.prev_log_idx_, cur_undolog);
        } else {
          auto new_undo_link = cur_txn->AppendUndoLog({false, modified_field, parital_tuple, tmp_ts, prev_link});
          txn_mgr->UpdateVersionLink(old_rid, VersionUndoLink{new_undo_link});
        }

      } else {
        auto new_link = cur_txn->AppendUndoLog({false, modified_field, old_tuple, tmp_ts, {}});
        txn_mgr->UpdateUndoLink(old_rid, new_link);
      }
    }
  }
  // TxnMgrDbg("Update后的结果", txn_mgr, table_info_, table_info_->table_.get());
  std::vector<Value> res = {{TypeId::INTEGER, cnt}};
  *tuple = Tuple(res, &plan_->OutputSchema());
  return true;
}

}  // namespace bustub
