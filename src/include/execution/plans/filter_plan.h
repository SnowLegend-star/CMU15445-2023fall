//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// filter_plan.h
//
// Identification: src/include/execution/plans/filter_plan.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <utility>

#include "catalog/catalog.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {

/**
 * FilterPlanNode 表示一个过滤操作。
 * 它保留任何满足子节点谓词的元组。
 */
 class FilterPlanNode : public AbstractPlanNode {
  public:
   /**
    * 构造一个新的 FilterPlanNode 实例。
    * @param output 此过滤器计划节点的输出模式
    * @param predicate 在扫描操作中应用的谓词
    * @param child 子计划节点
    */
   FilterPlanNode(SchemaRef output, AbstractExpressionRef predicate, AbstractPlanNodeRef child)
       : AbstractPlanNode(std::move(output), {std::move(child)}), predicate_{std::move(predicate)} {}
 
   /** @return 计划节点的类型 */
   auto GetType() const -> PlanType override { return PlanType::Filter; }
 
   /** @return 用于测试元组的谓词；只有当谓词为 true 时才会返回元组 */
   auto GetPredicate() const -> const AbstractExpressionRef & { return predicate_; }
 
   /** @return 子计划节点 */
   auto GetChildPlan() const -> AbstractPlanNodeRef {
     BUSTUB_ASSERT(GetChildren().size() == 1, "Filter 应该只有一个子计划节点。");
     return GetChildAt(0);
   }
 
   BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(FilterPlanNode);
 
   /** 所有返回的元组必须满足的谓词 */
   AbstractExpressionRef predicate_;
 
  protected:
   auto PlanNodeToString() const -> std::string override {
     return fmt::format("Filter {{ predicate={} }}", *predicate_);
   }
 }; 

}  // namespace bustub
