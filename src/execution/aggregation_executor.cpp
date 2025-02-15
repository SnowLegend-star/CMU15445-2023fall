//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "common/rid.h"
#include "execution/executors/aggregation_executor.h"
#include "execution/plans/aggregation_plan.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)),
      aht_(plan->GetAggregates(), plan->GetAggregateTypes()),
      aht_iterator_(aht_.Begin()) {
  // aht_ = SimpleAggregationHashTable(plan->GetAggregates(), plan->GetAggregateTypes());  // 赋值操作有问题
}

void AggregationExecutor::Init() {
  std::cout << plan_->ToString() << std::endl;
  child_executor_->Init();
  Tuple child_tuple{};
  RID child_rid{};
  auto group_bys = plan_->group_bys_;
  auto group_aggrs = plan_->aggregates_;
  auto aggrs_types = plan_->agg_types_;
  // 从child_exec获得元组, 并进行聚合
  aht_.Clear();
  while (child_executor_->Next(&child_tuple, &child_rid)) {
    auto aggr_key = MakeAggregateKey(&child_tuple);
    AggregateValue aggr_val = MakeAggregateValue(&child_tuple);
    aht_.InsertCombine(aggr_key, aggr_val);
  }  // 现在所有的元组都已经在aht_内了

  aht_iterator_ = aht_.Begin();
  // 如果table是空的, 且聚合函数只有count(*)才需要这么写
  if (aht_iterator_ == aht_.End() && plan_->output_schema_->GetColumnCount() == 1) {  // 空的哈希表也要初始化一遍
    aht_.InsertEmptyCombine();
  }
  aht_iterator_ = aht_.Begin();  // 必须重新初始化一遍
}

// 这里应该是遍历aht_获得元组
auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (aht_iterator_ == aht_.End()) {
    return false;
  }

  auto aggr_key = aht_iterator_.Key();
  auto aggr_value = aht_iterator_.Val();
  std::vector<Value> result;
  // group by也要生成一个新的列
  for (auto &elem : aggr_key.group_bys_) {
    result.emplace_back(elem);
  }
  // 把aggregates_对应的值都取出来形成value集合
  for (auto &elem : aggr_value.aggregates_) {
    result.emplace_back(elem);
  }
  *tuple = Tuple(result, &GetOutputSchema());
  *rid = tuple->GetRid();
  ++aht_iterator_;
  return true;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_executor_.get(); }

}  // namespace bustub
