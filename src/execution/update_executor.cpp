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
#include <memory>
#include <optional>
#include <vector>

#include "common/rid.h"
#include "execution/executors/update_executor.h"
#include "execution/plans/update_plan.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value.h"

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
  Tuple tmp_tuple{};
  RID tmp_rid{};
  while (child_executor_->Next(&tmp_tuple, &tmp_rid)) {
    cnt++;
    // 先将旧的tuple删除再插入更新的tuple
    table_info_->table_->UpdateTupleMeta(TupleMeta{0, true}, tmp_rid);
    std::vector<Value> new_value{};
    new_value.reserve(plan_->target_expressions_.size());  // 其实这个new_value和原value差不多
    for (const auto &attr : plan_->target_expressions_) {  // 把new_value填充为完整的value
      new_value.push_back(attr->Evaluate(&tmp_tuple, child_executor_->GetOutputSchema()));
    }
    auto update_tuple = Tuple{new_value, &table_info_->schema_};  // 用value构造完整的tuple
    // 插入新元组
    std::optional<RID> new_rid_optional = table_info_->table_->InsertTuple(TupleMeta{0, false}, update_tuple);
    RID new_rid = new_rid_optional.value();
    for (auto &index_info : table_indexes) {
      auto index = index_info->index_.get();
      auto key_attrs = index_info->index_->GetKeyAttrs();
      auto old_key = tmp_tuple.KeyFromTuple(table_info_->schema_, *index->GetKeySchema(), key_attrs);
      auto new_key = update_tuple.KeyFromTuple(table_info_->schema_, *index->GetKeySchema(), key_attrs);
      // 删除旧的索引
      index->DeleteEntry(old_key, tmp_rid, exec_ctx_->GetTransaction());
      // 插入新的索引
      index->InsertEntry(new_key, new_rid, exec_ctx_->GetTransaction());
    }
  }
  std::vector<Value> res = {{TypeId::INTEGER, cnt}};
  *tuple = Tuple(res, &plan_->OutputSchema());
  return true;
}

}  // namespace bustub
