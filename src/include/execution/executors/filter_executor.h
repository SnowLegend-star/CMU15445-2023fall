//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// filter_executor.h
//
// Identification: src/include/execution/executors/filter_executor.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * FilterExecutor 执行一个过滤器。
 */
 class FilterExecutor : public AbstractExecutor {
  public:
   /**
    * 构造一个新的 FilterExecutor 实例。
    * @param exec_ctx 执行上下文
    * @param plan 要执行的过滤器计划
    * @param child_executor 提供过滤器输入的子执行器
    */
   FilterExecutor(ExecutorContext *exec_ctx, const FilterPlanNode *plan,
                  std::unique_ptr<AbstractExecutor> &&child_executor);
 
   /** 初始化过滤器 */
   void Init() override;
 
   /**
    * 输出过滤器的下一个元组。
    * @param[out] tuple 过滤器生成的下一个元组
    * @param[out] rid 过滤器生成的下一个元组的 RID
    * @return 如果生成了元组，返回 `true`；如果没有更多元组，返回 `false`
    */
   auto Next(Tuple *tuple, RID *rid) -> bool override;
 
   /** @return 过滤器计划的输出模式 */
   auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }
 
  private:
   /** 要执行的过滤器计划节点 */
   const FilterPlanNode *plan_;
 
   /** 用于提供元组的子执行器 */
   std::unique_ptr<AbstractExecutor> child_executor_;
 }; 
}  // namespace bustub
