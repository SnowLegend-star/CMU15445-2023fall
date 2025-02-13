#include <memory>
#include <utility>
#include <vector>
#include "execution/plans/abstract_plan.h"
#include "execution/plans/limit_plan.h"
#include "execution/plans/sort_plan.h"
#include "execution/plans/topn_plan.h"
#include "optimizer/optimizer.h"

namespace bustub {

auto Optimizer::OptimizeSortLimitAsTopN(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement sort + limit -> top N optimizer rule
  std::vector<AbstractPlanNodeRef> children;
  for (auto &child : plan->children_) {
    children.emplace_back(OptimizeSortLimitAsTopN(child));
  }
  auto optimized_plan = plan->CloneWithChildren(std::move(children));
  if (optimized_plan->GetType() == PlanType::Limit) {
    auto limit_plan = dynamic_cast<LimitPlanNode *>(optimized_plan.get());
    auto child_plan = limit_plan->GetChildPlan();  // limit只能有一个child
    if (child_plan->GetType() == PlanType::Sort) {
      const auto sort_plan = dynamic_cast<const SortPlanNode *>(child_plan.get());
      if (sort_plan != nullptr) {
        return std::make_shared<TopNPlanNode>(optimized_plan->output_schema_, child_plan->GetChildren()[0],
                                              sort_plan->order_bys_, limit_plan->limit_);
      }
    }
  }
  return optimized_plan;
}

}  // namespace bustub
