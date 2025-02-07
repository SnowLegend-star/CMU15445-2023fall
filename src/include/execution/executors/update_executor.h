//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.h
//
// Identification: src/include/execution/executors/update_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/update_plan.h"
#include "storage/table/tuple.h"
#include "type/value_factory.h"

namespace bustub {

/**
 * UpdateExecutor 执行对表的更新操作。
 * 更新的值总是从子执行器中获取。
 */
class UpdateExecutor : public AbstractExecutor {
  friend class UpdatePlanNode;

 public:
  /**
   * 构造一个新的 UpdateExecutor 实例。
   * @param exec_ctx 执行上下文
   * @param plan 要执行的更新计划
   * @param child_executor 提供更新数据的子执行器
   */
  UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  /** 初始化更新 */
  void Init() override;

  /**
   * 获取更新的下一个元组。
   * @param[out] tuple 更新操作产生的下一个元组
   * @param[out] rid 更新操作产生的下一个元组的 RID（此参数忽略）
   * @return `true` 如果有元组产生，`false` 如果没有更多元组
   *
   * 注意：UpdateExecutor::Next() 不使用 `rid` 输出参数。
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return 更新操作的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** 要执行的更新计划节点 */
  const UpdatePlanNode *plan_;

  /** 标识需要更新的表的元数据 */
  const TableInfo *table_info_;

  /** 用于获取更新值的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;
  bool update_finish_;
};
}  // namespace bustub
