//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// values_executor.h
//
// Identification: src/include/execution/executors/values_executor.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/seq_scan_plan.h"
#include "execution/plans/values_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * ValuesExecutor 执行器用于生成值的行。
 */
class ValuesExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 ValuesExecutor 实例。
   * @param exec_ctx 执行器上下文
   * @param plan 要执行的 values 计划
   */
  ValuesExecutor(ExecutorContext *exec_ctx, const ValuesPlanNode *plan);

  /** 初始化 values */
  void Init() override;

  /**
   * 从 values 中产生下一行元组。
   * @param[out] tuple 由 values 产生的下一行元组
   * @param[out] rid 由 values 产生的下一行元组的 RID，values 执行器不使用该字段
   * @return 如果产生了元组则返回 `true`，如果没有更多元组则返回 `false`
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return values 计划的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** 要执行的 values 计划节点 */
  const ValuesPlanNode *plan_;

  const Schema dummy_schema_;

  size_t cursor_{0};
};
}  // namespace bustub
