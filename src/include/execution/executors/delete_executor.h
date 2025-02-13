//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.h
//
// Identification: src/include/execution/executors/delete_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/delete_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * DeleteExecutor 执行一个表的删除操作。
 * 被删除的值总是从子执行器中获取。
 */
class DeleteExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 DeleteExecutor 实例。
   * @param exec_ctx 执行器上下文
   * @param plan 要执行的删除计划
   * @param child_executor 提供删除操作的子执行器
   */
  DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  /** 初始化删除操作 */
  void Init() override;

  /**
   * 返回删除的行数。
   * @param[out] tuple 一个整数元组，表示删除的行数
   * @param[out] rid 删除操作生成的下一个元组的RID（忽略，不使用）
   * @return 如果产生了一个元组，返回 `true`，如果没有更多元组，返回 `false`
   *
   * 注意：DeleteExecutor::Next() 仅在删除操作执行一次后返回删除的行数。
   */
  auto Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool override;

  /** @return 删除操作的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  /** 要执行的删除计划节点 */
  const DeletePlanNode *plan_;

  /** 从中获取删除元组 RID 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;
  bool delete_finish_;  //手动设置只执行一次的flag
};
}  // namespace bustub
