//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.h
//
// Identification: src/include/execution/executors/insert_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/insert_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * InsertExecutor 执行在表上的插入操作。
 * 插入的值始终来自子执行器。
 */
class InsertExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 InsertExecutor 实例。
   * @param exec_ctx 执行器上下文
   * @param plan 要执行的插入计划
   * @param child_executor 从中提取插入元组的子执行器
   */
  InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  /** 初始化插入操作 */
  void Init() override;

  /**
   * 返回插入到表中的行数。
   * @param[out] tuple 一个整数元组，表示插入到表中的行数
   * @param[out] rid 插入操作产生的下一个元组 RID（忽略，不使用）
   * @return 如果生成了元组，则返回 `true`；如果没有更多元组，则返回 `false`
   *
   * 注意：InsertExecutor::Next() 不使用 `rid` 输出参数。
   * 注意：InsertExecutor::Next() 仅在插入的行数产生时返回 `true`。
   */
  auto Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool override;

  /** @return 插入操作的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  /** 要执行的插入计划节点 */
  const InsertPlanNode *plan_;
  /** The child executor from which tuples are obtained */
  std::unique_ptr<AbstractExecutor> child_executor_;
  // 插入结束后得手动终止
  bool insert_finish_;
};

}  // namespace bustub
