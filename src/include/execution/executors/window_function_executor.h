//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// window_function_executor.h
//
// Identification: src/include/execution/executors/window_function_executor.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/aggregation_plan.h"
#include "execution/plans/window_plan.h"
#include "storage/table/tuple.h"
#include "type/type.h"
#include "type/type_id.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {
// 自定义比较类
class CompareTuple {
 public:
  CompareTuple(const std::vector<std::pair<OrderByType, AbstractExpressionRef>> *order_bys, const Schema *schema) {
    order_bys_ = order_bys;
    schema_ = schema;
  }

  const std::vector<std::pair<OrderByType, AbstractExpressionRef>> *order_bys_;
  const Schema *schema_;

  auto operator()(std::pair<Tuple, RID> &a, std::pair<Tuple, RID> &b) const -> bool {
    for (const auto &order_by : (*order_bys_)) {
      auto value_a = order_by.second->Evaluate(&(a.first), *schema_);
      auto value_b = order_by.second->Evaluate(&(b.first), *schema_);
      if (order_by.first == OrderByType::DESC) {
        if (value_a.CompareGreaterThan(value_b) == CmpBool::CmpTrue) {
          return true;
        }
        if (value_a.CompareLessThan(value_b) == CmpBool::CmpTrue) {
          return false;
        }
      } else if (order_by.first == OrderByType::ASC || order_by.first == OrderByType::DEFAULT) {
        if (value_a.CompareGreaterThan(value_b) == CmpBool::CmpTrue) {
          return false;
        }
        if (value_a.CompareLessThan(value_b) == CmpBool::CmpTrue) {
          return true;
        }
      }
    }
    return true;
  }
};

/**
 * 一个简化的哈希表，具有进行聚合操作所需的所有功能。
 */
class SimpleWindowFunctionHashTable {
 public:
  auto MapWindowTypeToInteger(WindowFunctionType type) -> int {
    int value = 0;
    switch (type) {
      case WindowFunctionType::CountStarAggregate:
        value = 0;
        break;
      case WindowFunctionType::CountAggregate:
        value = 1;
        break;
      case WindowFunctionType::SumAggregate:
        value = 2;
        break;
      case WindowFunctionType::MinAggregate:
        value = 3;
        break;
      case WindowFunctionType::MaxAggregate:
        value = 4;
        break;
      case WindowFunctionType::Rank:
        value = 5;
        break;
    }
    return value;
  }

  auto GenerateInitialPartitionValue() -> AggregateValue {
    // 默认包含所有种类的窗口函数
    std::vector<Value> values;
    values.push_back(ValueFactory::GetIntegerValue(0));
    values.push_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
    values.push_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
    values.push_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
    values.push_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
    values.push_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
    return {values};
  }

  void InsertCombine(const AggregateKey &key, const Value &value, WindowFunctionType type) {
    if (window_map_.count(key) == 0) {
      window_map_.insert({key, GenerateInitialPartitionValue()});
    }
    CombineAggregateValues(&window_map_[key], value, type);
  }

  void CombineAggregateValues(AggregateValue *result, const Value &input, WindowFunctionType type) {
    int type_id = MapWindowTypeToInteger(type);
    switch (type) {
      case WindowFunctionType::CountStarAggregate:
        result->aggregates_[type_id] = result->aggregates_[type_id].Add(Value(INTEGER, 1));
        break;
      case WindowFunctionType::CountAggregate:
        if (!input.IsNull()) {
          if (result->aggregates_[type_id].IsNull()) {
            result->aggregates_[type_id] = Value(INTEGER, 0);
          }
          result->aggregates_[type_id] = result->aggregates_[type_id].Add(Value(INTEGER, 1));
        }
        break;
      case WindowFunctionType::SumAggregate:
        if (!input.IsNull()) {
          if (result->aggregates_[type_id].IsNull()) {
            result->aggregates_[type_id] = Value(INTEGER, 0);
          }
          result->aggregates_[type_id] = result->aggregates_[type_id].Add(input);
        }
        break;
      case WindowFunctionType::MinAggregate:
        if (!input.IsNull()) {
          if (result->aggregates_[type_id].IsNull()) {
            result->aggregates_[type_id] = input;
          } else {
            result->aggregates_[type_id] = result->aggregates_[type_id].Min(input);
          }
        }
        break;

      case WindowFunctionType::MaxAggregate:
        if (!input.IsNull()) {
          if (result->aggregates_[type_id].IsNull()) {
            result->aggregates_[type_id] = input;
          } else {
            result->aggregates_[type_id] = result->aggregates_[type_id].Max(input);
          }
        }
        break;
      case WindowFunctionType::Rank:
        if (!input.IsNull()) {
          if (result->aggregates_[type_id].IsNull()) {
            result->aggregates_[type_id] = Value(INTEGER, 0);
          }
          result->aggregates_[type_id] = result->aggregates_[type_id].Add(Value(INTEGER, 1));
        }
        break;
    }
  }

  auto GetValueFromMap(AggregateKey &key, WindowFunctionType type) -> Value {
    return window_map_[key].aggregates_[MapWindowTypeToInteger(type)];
  }

 private:
  std::unordered_map<AggregateKey, AggregateValue> window_map_;
};

/**
 * WindowFunctionExecutor 执行具有窗口函数的列的窗口函数。
 *
 * 窗口函数与普通的聚合函数不同，它为每个输入的行输出一行，并且可以与普通选择的列结合使用。
 * 在 `WindowFunctionPlanNode` 中，列包含了普通选择的列和窗口函数的占位符列。
 *
 * 例如，如果我们有如下查询：
 *    SELECT 0.1, 0.2, SUM(0.3) OVER (PARTITION BY 0.2 ORDER BY 0.3), SUM(0.4) OVER (PARTITION BY 0.1 ORDER BY 0.2,0.3)
 *      FROM table;
 *
 * `WindowFunctionPlanNode` 的结构如下：
 *    columns: std::vector<AbstractExpressionRef>{0.1, 0.2, 0.-1(占位符), 0.-1(占位符)}
 *    window_functions_: {
 *      3: {
 *        partition_by: std::vector<AbstractExpressionRef>{0.2}
 *        order_by: std::vector<AbstractExpressionRef>{0.3}
 *        functions: std::vector<AbstractExpressionRef>{0.3}  什么傻逼注释,
 * 这tm应该是一个单纯的AbstractExpressionRef好不好，写牛魔的vector呢??! window_func_type:
 * WindowFunctionType::SumAggregate
 *      }
 *      4: {
 *        partition_by: std::vector<AbstractExpressionRef>{0.1}
 *        order_by: std::vector<AbstractExpressionRef>{0.2,0.3}
 *        functions: std::vector<AbstractExpressionRef>{0.4}
 *        window_func_type: WindowFunctionType::SumAggregate
 *      }
 *    }
 *
 * 你的执行器应该使用子执行器和 columns 中的表达式来生成除窗口函数列外的选择列，并使用
 * window_agg_indexes、partition_bys、order_bys、functions 和 window_agg_types 来生成窗口函数列的结果。 直接在 columns
 * 中使用占位符来表示窗口函数列是不允许的，因为它包含无效的列 ID。
 *
 * 你的 `WindowFunctionExecutor` 不需要支持指定的窗口框架（例如：1 行前和 1 行后）。
 * 你可以假设所有的窗口框架在有 `ORDER BY` 子句时是 **UNBOUNDED FOLLOWING AND CURRENT ROW**，没有 `ORDER BY` 子句时是
 * **UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING**。
 *
 */
class WindowFunctionExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 WindowFunctionExecutor 实例。
   * @param exec_ctx 执行上下文
   * @param plan 要执行的窗口聚合计划
   */
  WindowFunctionExecutor(ExecutorContext *exec_ctx, const WindowFunctionPlanNode *plan,
                         std::unique_ptr<AbstractExecutor> &&child_executor);

  /** 初始化窗口聚合 */
  void Init() override;

  /**
   * 输出窗口聚合的下一个元组。
   * @param[out] tuple 窗口聚合生成的下一个元组
   * @param[out]
   * @param[out] rid 窗口聚合生成的下一个元组的 RID
   * @return 如果生成了元组，则返回 `true`；如果没有更多元组，则返回 `false`
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return 窗口聚合计划的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** @return 元组作为聚合键 */
  // 这里如果group by是空的, 那keys就是空的
  auto MakeWindowFunctionKey(const Tuple *tuple, const std::vector<AbstractExpressionRef> &partition_by)
      -> AggregateKey {
    std::vector<Value> keys;
    keys.reserve(partition_by.size());
    for (const auto &expr : partition_by) {
      keys.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {keys};
  }

  /** @return 元组作为聚合值 */
  auto MakeWindowFunctionValue(const Tuple *tuple, const AbstractExpressionRef &window_func) -> Value {
    Value val = window_func->Evaluate(tuple, child_executor_->GetOutputSchema());
    return val;
  }
  /** 要执行的窗口聚合计划节点 */
  const WindowFunctionPlanNode *plan_;

  /** 用于获取元组的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  std::vector<std::pair<Tuple, RID>> tuples_;
  std::vector<std::pair<Tuple, RID>>::iterator it_;
  /** 每个window_func都分配一个哈希表**/
  SimpleWindowFunctionHashTable hash_table_;

  bool having_order_by_ = false;
};

}  // namespace bustub
