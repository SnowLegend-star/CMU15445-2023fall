//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_plan.h
//
// Identification: src/include/execution/plans/aggregation_plan.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"

namespace bustub {

/** AggregationType 枚举列举了系统中所有可能的聚合函数 */
enum class AggregationType { CountStarAggregate, CountAggregate, SumAggregate, MinAggregate, MaxAggregate };

/**
 * AggregationPlanNode 表示各种 SQL 聚合函数。
 * 例如，COUNT()、SUM()、MIN() 和 MAX()。
 *
 * 注意：为了简化该项目，AggregationPlanNode 必须始终只有一个子节点。
 * NOTE: To simplify this project, AggregationPlanNode must always have exactly one child.
 */
class AggregationPlanNode : public AbstractPlanNode {
 public:
  /**
   * 构造一个新的 AggregationPlanNode 实例。
   * @param output_schema 输出格式的计划节点
   * @param child 聚合数据的子计划
   * @param group_bys 聚合的 GROUP BY 子句
   * @param aggregates 我们正在进行聚合的表达式
   * @param agg_types 我们聚合的类型
   */
  AggregationPlanNode(SchemaRef output_schema, AbstractPlanNodeRef child, std::vector<AbstractExpressionRef> group_bys,
                      std::vector<AbstractExpressionRef> aggregates, std::vector<AggregationType> agg_types)
      : AbstractPlanNode(std::move(output_schema), {std::move(child)}),
        group_bys_(std::move(group_bys)),
        aggregates_(std::move(aggregates)),
        agg_types_(std::move(agg_types)) {}

  /** @return 计划节点的类型 */
  auto GetType() const -> PlanType override { return PlanType::Aggregation; }

  /** @return 该聚合计划节点的子节点 */
  /** AggregationPlanNode 应该只有一个子节点 **/
  auto GetChildPlan() const -> AbstractPlanNodeRef {
    BUSTUB_ASSERT(GetChildren().size() == 1, "Aggregation expected to only have one child.");
    return GetChildAt(0);
  }

  /** @return 第 idx 个 GROUP BY 表达式 */
  auto GetGroupByAt(uint32_t idx) const -> const AbstractExpressionRef & { return group_bys_[idx]; }

  /** @return GROUP BY 表达式的集合 */
  auto GetGroupBys() const -> const std::vector<AbstractExpressionRef> & { return group_bys_; }

  /** @return 第 idx 个聚合表达式 */
  auto GetAggregateAt(uint32_t idx) const -> const AbstractExpressionRef & { return aggregates_[idx]; }

  /** @return 聚合表达式的集合 */
  auto GetAggregates() const -> const std::vector<AbstractExpressionRef> & { return aggregates_; }

  /** @return 聚合类型的集合 */
  auto GetAggregateTypes() const -> const std::vector<AggregationType> & { return agg_types_; }

  static auto InferAggSchema(const std::vector<AbstractExpressionRef> &group_bys,
                             const std::vector<AbstractExpressionRef> &aggregates,
                             const std::vector<AggregationType> &agg_types) -> Schema;

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(AggregationPlanNode);

  /** GROUP BY 表达式 */
  std::vector<AbstractExpressionRef> group_bys_;
  /** 聚合表达式 */
  std::vector<AbstractExpressionRef> aggregates_;
  /** 聚合类型 */
  std::vector<AggregationType> agg_types_;

 protected:
  auto PlanNodeToString() const -> std::string override;
};

/** AggregateKey 表示聚合操作中的一个键 */
struct AggregateKey {
  /** GROUP BY 值 */
  std::vector<Value> group_bys_;

  /**
   * 比较两个聚合键是否相等。
   * @param other 另一个聚合键进行比较
   * @return 如果两个聚合键的 GROUP BY 表达式相同，则返回 `true`，否则返回 `false`
   */
  auto operator==(const AggregateKey &other) const -> bool {
    for (uint32_t i = 0; i < other.group_bys_.size(); i++) {
      if (group_bys_[i].CompareEquals(other.group_bys_[i]) != CmpBool::CmpTrue) {
        return false;
      }
    }
    return true;
  }
};

/** AggregateValue 表示每个运行中的聚合的值 */
struct AggregateValue {
  /** 聚合值集合 */
  std::vector<Value> aggregates_;
};

}  // namespace bustub

namespace std {

///** 为 AggregateKey 实现 std::hash */
template <>
struct hash<bustub::AggregateKey> {
  auto operator()(const bustub::AggregateKey &agg_key) const -> std::size_t {
    size_t curr_hash = 0;
    for (const auto &key : agg_key.group_bys_) {
      if (!key.IsNull()) {
        curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&key));
      }
    }
    return curr_hash;
  }
};

}  // namespace std

template <>
///** 为 AggregationType 提供 fmt::formatter 支持 */
struct fmt::formatter<bustub::AggregationType> : formatter<std::string> {
  template <typename FormatContext>
  auto format(bustub::AggregationType c, FormatContext &ctx) const {
    using bustub::AggregationType;
    std::string name = "unknown";
    switch (c) {
      case AggregationType::CountStarAggregate:
        name = "count_star";
        break;
      case AggregationType::CountAggregate:
        name = "count";
        break;
      case AggregationType::SumAggregate:
        name = "sum";
        break;
      case AggregationType::MinAggregate:
        name = "min";
        break;
      case AggregationType::MaxAggregate:
        name = "max";
        break;
    }
    return formatter<std::string>::format(name, ctx);
  }
};
