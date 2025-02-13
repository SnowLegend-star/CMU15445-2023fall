#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "common/macros.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/hash_join_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "execution/plans/projection_plan.h"
#include "optimizer/optimizer.h"
#include "type/type_id.h"

namespace bustub {

// 拆分谓词中的表达式
// 本质上, 表达式的基本单位是column_value_expression
void Optimizer::PredParser(const AbstractExpressionRef &predicate, std::vector<AbstractExpressionRef> *left_pred,
                           std::vector<AbstractExpressionRef> *right_pred) {
  // 拆分逻辑表达式
  auto logic_expr = dynamic_cast<LogicExpression *>(predicate.get());
  if (logic_expr != nullptr) {
    PredParser(logic_expr->children_[0], left_pred, right_pred);  // 拆分逻辑表达式的左半部分 (#0.0=#1.0)
    PredParser(logic_expr->children_[1], left_pred, right_pred);  // 拆分逻辑表达式的右半部分 (#0.1=#1.2)
  }

  // 拆分cmp表达式
  // 注意这里要区分是从哪个table提取的column
  auto cmp_expr = dynamic_cast<ComparisonExpression *>(predicate.get());
  if (cmp_expr != nullptr) {
    auto tmp = dynamic_cast<ColumnValueExpression *>(cmp_expr->children_[0].get());
    if (tmp->GetTupleIdx() == 0) {  // 左侧column元素正是从left table提取的
      left_pred->emplace_back(cmp_expr->children_[0]);
      right_pred->emplace_back(cmp_expr->children_[1]);
    } else if (tmp->GetTupleIdx() == 1) {  // 左侧column元素正是从right table提取的
      left_pred->emplace_back(cmp_expr->children_[1]);
      right_pred->emplace_back(cmp_expr->children_[0]);
    }
  }
}

// 这里要注意一点, cmpExpr的比较类型默认是“=”
// 要较真的话得判断每个cmpExpr的比较类型
auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement NestedLoopJoin -> HashJoin optimizer rule
  // 2023 秋季学期注意：你应该支持任意数量的等值条件联合作为连接键：
  // 例如：<列表达式> = <列表达式> AND <列表达式> = <列表达式> AND ...

  std::vector<AbstractPlanNodeRef> children;
  for (auto &expr : plan->children_) {  // 递归优化
    children.emplace_back(OptimizeNLJAsHashJoin(expr));
  }

  auto optimized_plan = plan->CloneWithChildren(std::move(children));
  if (optimized_plan->GetType() == PlanType::NestedLoopJoin) {                       // 针对NLJ进行优化
    const auto &nlj_plan = dynamic_cast<NestedLoopJoinPlanNode &>(*optimized_plan);  // 将plan类型转化为HashJoin
    BUSTUB_ASSERT(nlj_plan.children_.size() == 2, "nlj计划应该拥有2个子节点");
    // 哈希连接的核心就是谓词条件是“=”
    std::vector<AbstractExpressionRef> left_expr;
    std::vector<AbstractExpressionRef> right_expr;
    PredParser(nlj_plan.predicate_, &left_expr, &right_expr);
    return std::make_shared<HashJoinPlanNode>(optimized_plan->output_schema_, optimized_plan->children_[0],
                                              optimized_plan->children_[1], left_expr, right_expr, nlj_plan.join_type_);
  }
  // return plan; // 千算万算没想到是这里给自己挖了个坑
  return optimized_plan;
}

}  // namespace bustub
