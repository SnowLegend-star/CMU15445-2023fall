//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.h
//
// Identification: src/include/execution/executors/aggregation_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "container/hash/hash_function.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/aggregation_plan.h"
#include "storage/table/tuple.h"
#include "type/type.h"
#include "type/type_id.h"
#include "type/value_factory.h"

namespace bustub {

/**
 * 一个简化的哈希表，具有进行聚合操作所需的所有功能。
 */
class SimpleAggregationHashTable {
 public:
  /**
   * 构造一个新的 SimpleAggregationHashTable 实例。
   * @param agg_exprs 聚合表达式
   * @param agg_types 聚合类型
   */
  SimpleAggregationHashTable(const std::vector<AbstractExpressionRef> &agg_exprs,
                             const std::vector<AggregationType> &agg_types)
      : agg_exprs_{agg_exprs}, agg_types_{agg_types} {}

  /** @return 为聚合执行器生成的初始聚合值 */
  auto GenerateInitialAggregateValue() -> AggregateValue {
    std::vector<Value> values{};
    for (const auto &agg_type : agg_types_) {
      switch (agg_type) {
        case AggregationType::CountStarAggregate:
          // Count star 从零开始。
          values.emplace_back(ValueFactory::GetIntegerValue(0));
          break;
        case AggregationType::CountAggregate:
        case AggregationType::SumAggregate:
        case AggregationType::MinAggregate:
        case AggregationType::MaxAggregate:
          // 其他聚合从 null 开始。
          values.emplace_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
          break;
      }
    }
    return {values};
  }

  /**
   * TODO(学生)
   *
   * 将输入合并到聚合结果中。
   * @param[out] result 输出的聚合值
   * @param input 输入值
   */
  void CombineAggregateValues(AggregateValue *result, const AggregateValue &input) {
    for (uint32_t i = 0; i < agg_exprs_.size(); i++) {
      Value old_value = result->aggregates_[i];
      switch (agg_types_[i]) {
        case AggregationType::CountStarAggregate:
          result->aggregates_[i] = result->aggregates_[i].Add(ValueFactory::GetIntegerValue(1));
          break;
        case AggregationType::CountAggregate:
          if (old_value.IsNull()) {
            result->aggregates_[i] = Value{TypeId::INTEGER, 0};
          }
          if (!input.aggregates_[i].IsNull()) {
            result->aggregates_[i] = result->aggregates_[i].Add(ValueFactory::GetIntegerValue(1));
          }
          break;
        case AggregationType::SumAggregate:
          if (old_value.IsNull()) {
            result->aggregates_[i] = input.aggregates_[i];
          } else if (!input.aggregates_[i].IsNull()) {
            result->aggregates_[i] = result->aggregates_[i].Add(input.aggregates_[i]);
          }
          break;
        case AggregationType::MinAggregate:
          if (old_value.IsNull()) {
            result->aggregates_[i] = input.aggregates_[i];
          } else if (!input.aggregates_[i].IsNull()) {
            result->aggregates_[i] = old_value.Min(input.aggregates_[i]);
          }
          break;
        case AggregationType::MaxAggregate:
          if (old_value.IsNull()) {
            result->aggregates_[i] = input.aggregates_[i];
          } else if (!input.aggregates_[i].IsNull()) {
            result->aggregates_[i] = old_value.Max(input.aggregates_[i]);
          }
          break;
      }
    }
  }

  /**
   * 将一个值插入哈希表并与当前聚合值合并。
   * @param agg_key 要插入的键
   * @param agg_val 要插入的值
   */
  void InsertCombine(const AggregateKey &agg_key, const AggregateValue &agg_val) {
    if (ht_.count(agg_key) == 0) {
      ht_.insert({agg_key, GenerateInitialAggregateValue()});
    }
    CombineAggregateValues(&ht_[agg_key], agg_val);
  }
  void InsertEmptyCombine() { ht_.insert({{std::vector<Value>()}, GenerateInitialAggregateValue()}); }
  /**
   * 清空哈希表
   */
  void Clear() { ht_.clear(); }

  /** 聚合哈希表的迭代器 */
  class Iterator {
   public:
    /** 为聚合映射创建一个迭代器。 */
    explicit Iterator(std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter) : iter_{iter} {}

    /** @return 迭代器的键 */
    auto Key() -> const AggregateKey & { return iter_->first; }

    /** @return 迭代器的值 */
    auto Val() -> const AggregateValue & { return iter_->second; }

    /** @return 迭代器递增前的迭代器 */
    auto operator++() -> Iterator & {
      ++iter_;
      return *this;
    }

    /** @return `true` 如果两个迭代器相等 */
    auto operator==(const Iterator &other) -> bool { return this->iter_ == other.iter_; }

    /** @return `true` 如果两个迭代器不相等 */
    auto operator!=(const Iterator &other) -> bool { return this->iter_ != other.iter_; }

   private:
    /** 聚合映射 */
    std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter_;
  };

  /** @return 哈希表的开始迭代器 */
  auto Begin() -> Iterator { return Iterator{ht_.cbegin()}; }

  /** @return 哈希表的结束迭代器 */
  auto End() -> Iterator { return Iterator{ht_.cend()}; }

  /** 获得哈希表当前的大小**/
  auto Size() -> size_t { return ht_.size(); }

 private:
  /** 哈希表是一个从聚合键到聚合值的映射 */
  std::unordered_map<AggregateKey, AggregateValue> ht_{};
  /** 我们的聚合表达式 */
  const std::vector<AbstractExpressionRef> &agg_exprs_;
  /** 我们的聚合类型 */
  const std::vector<AggregationType> &agg_types_;
};

/**
 * 聚合执行器执行一个聚合操作（例如 COUNT、SUM、MIN、MAX）
 * 对子执行器生成的元组进行聚合。
 */
class AggregationExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 AggregationExecutor 实例。
   * @param exec_ctx 执行上下文
   * @param plan 要执行的插入计划
   * @param child_executor 从中提取元组的子执行器（可能是 `nullptr`）
   */
  AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                      std::unique_ptr<AbstractExecutor> &&child_executor);

  /** 初始化聚合 */
  void Init() override;

  /**
   * 从插入中产生下一个元组。
   * @param[out] tuple 聚合产生的下一个元组
   * @param[out] rid 聚合产生的下一个元组 RID
   * @return `true` 如果产生了一个元组，`false` 如果没有更多元组
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return 聚合的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

  /** 不要使用或移除此函数，否则你将得零分。 */
  auto GetChildExecutor() const -> const AbstractExecutor *;

 private:
  /** @return 元组作为聚合键 */
  // 这里如果group by是空的, 那keys就是空的
  auto MakeAggregateKey(const Tuple *tuple) -> AggregateKey {
    std::vector<Value> keys;
    for (const auto &expr : plan_->GetGroupBys()) {
      keys.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {keys};
  }

  /** @return 元组作为聚合值 */
  auto MakeAggregateValue(const Tuple *tuple) -> AggregateValue {
    std::vector<Value> vals;
    for (const auto &expr : plan_->GetAggregates()) {
      vals.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {vals};
  }

 private:
  /** 聚合计划节点 */
  const AggregationPlanNode *plan_;

  /** 产生元组的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 简单聚合哈希表 */
  // TODO(学生): 取消注释
  SimpleAggregationHashTable aht_;

  /** 简单聚合哈希表迭代器 */
  // TODO(学生): 取消注释
  SimpleAggregationHashTable::Iterator aht_iterator_;
};
}  // namespace bustub
