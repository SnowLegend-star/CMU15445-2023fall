//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include "binder/table_ref/bound_join_ref.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "common/rid.h"
#include "storage/table/tuple.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)) {
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2023 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();
  std::cout << plan_->ToString() << std::endl;
  RID tmp_rid;
  left_status_ = left_executor_->Next(&left_cur_tuple_, &tmp_rid);
  tuple_match_ = false;
}

auto NestedLoopJoinExecutor::InnerJoin() -> bool { return true; }

auto NestedLoopJoinExecutor::LeftJoin() -> bool { return true; }

auto NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  auto filter_expr = plan_->predicate_;
  // Tuple left_tuple{};
  Tuple right_tuple{};
  RID left_rid{};
  RID right_rid{};
  Schema left_schema = plan_->GetLeftPlan()->OutputSchema();
  Schema right_schema = plan_->GetRightPlan()->OutputSchema();
  // right_executor_->Init();  //每次都要重置right table

  while (true) {
    if (!left_status_) {
      return false;
    }

    // 嵌套循环
    while (true) {
      auto right_status = right_executor_->Next(&right_tuple, &right_rid);
      if (!right_status) {  //已经找完right table的元素了
        // 对应left join, 开始添加null。对于inner, 则直接进入下一次循环
        // 如果这个left tuple在当前轮次已经找到过对应的right tuple就跳过这一步
        if (plan_->join_type_ == JoinType::LEFT && !tuple_match_) {
          std::vector<Value> result;
          Value cur_val;  // 当前列的value值
          // 提取左表所有的列元素
          for (uint32_t i = 0; i < left_schema.GetColumnCount(); i++) {
            cur_val = left_cur_tuple_.GetValue(&left_schema, i);
            result.emplace_back(cur_val);
          }
          // 右表没有匹配的tuple, 生成空值就行了
          for (uint32_t i = 0; i < right_schema.GetColumnCount(); i++) {
            cur_val = ValueFactory::GetNullValueByType(right_schema.GetColumn(i).GetType());
            result.emplace_back(cur_val);
          }
          *tuple = Tuple{result, &plan_->OutputSchema()};
          // tuple_match_=true;  // 这个left tuple遍历到right table的末尾了, 不算找到
          right_executor_->Init();
          left_status_ = left_executor_->Next(&left_cur_tuple_, &left_rid);
          return true;
        }
        right_executor_->Init();
        left_status_ = left_executor_->Next(&left_cur_tuple_, &left_rid);
        tuple_match_ = false;
        break;
      }
      // 判断两个tuple是否符合pred  value的类型为bool
      auto value = plan_->Predicate()->EvaluateJoin(&left_cur_tuple_, left_schema, &right_tuple, right_schema);

      if (!value.IsNull() && value.GetAs<bool>()) {  // 何时value会为空呢?我很好奇
        // 提取左表和右表所有的列元素
        std::vector<Value> result;
        Value cur_val;  // 当前列的value值
        for (uint32_t i = 0; i < left_schema.GetColumnCount(); i++) {
          cur_val = left_cur_tuple_.GetValue(&left_schema, i);
          result.emplace_back(cur_val);
        }
        for (uint32_t i = 0; i < right_schema.GetColumnCount(); i++) {
          cur_val = right_tuple.GetValue(&right_schema, i);
          result.emplace_back(cur_val);
        }
        *tuple = Tuple{result, &plan_->OutputSchema()};
        tuple_match_ = true;  // 这个left tuple已经找到对应的right tuple了
        return true;
      }
    }
  }

  return false;
}

}  // namespace bustub
