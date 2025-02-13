//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_plan.h
//
// Identification: src/include/execution/plans/hash_join_plan.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "binder/table_ref/bound_join_ref.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {

/**
 * 哈希连接执行使用哈希表进行连接操作。
 */
class HashJoinPlanNode : public AbstractPlanNode {
 public:
  /**
   * 构造一个新的 HashJoinPlanNode 实例。
   * @param output_schema 连接操作的输出模式
   * @param children 从中获取元组的子查询
   * @param left_key_expression 左侧连接键的表达式
   * @param right_key_expression 右侧连接键的表达式
   */
  HashJoinPlanNode(SchemaRef output_schema, AbstractPlanNodeRef left, AbstractPlanNodeRef right,
                   std::vector<AbstractExpressionRef> left_key_expressions,
                   std::vector<AbstractExpressionRef> right_key_expressions, JoinType join_type)
      : AbstractPlanNode(std::move(output_schema), {std::move(left), std::move(right)}),
        left_key_expressions_{std::move(left_key_expressions)},
        right_key_expressions_{std::move(right_key_expressions)},
        join_type_(join_type) {}

  /** @return 该计划节点的类型 */
  auto GetType() const -> PlanType override { return PlanType::HashJoin; }

  /** @return 用于计算左侧连接键的表达式 */
  auto LeftJoinKeyExpressions() const -> const std::vector<AbstractExpressionRef> & { return left_key_expressions_; }

  /** @return 用于计算右侧连接键的表达式 */
  auto RightJoinKeyExpressions() const -> const std::vector<AbstractExpressionRef> & { return right_key_expressions_; }

  /** @return 哈希连接的左侧计划节点 */
  auto GetLeftPlan() const -> AbstractPlanNodeRef {
    BUSTUB_ASSERT(GetChildren().size() == 2, "哈希连接应该有恰好两个子计划节点。");
    return GetChildAt(0);
  }

  /** @return 哈希连接的右侧计划节点 */
  auto GetRightPlan() const -> AbstractPlanNodeRef {
    BUSTUB_ASSERT(GetChildren().size() == 2, "哈希连接应该有恰好两个子计划节点。");
    return GetChildAt(1);
  }

  /** @return 哈希连接使用的连接类型 */
  auto GetJoinType() const -> JoinType { return join_type_; };

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(HashJoinPlanNode);

  /** 用于计算左侧连接键的表达式 */
  std::vector<AbstractExpressionRef> left_key_expressions_;
  /** 用于计算右侧连接键的表达式 */
  std::vector<AbstractExpressionRef> right_key_expressions_;

  /** 连接类型 */
  JoinType join_type_;

 protected:
  auto PlanNodeToString() const -> std::string override;
};

}  // namespace bustub
