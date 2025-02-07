//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// abstract_executor.h
//
// Identification: src/include/execution/executors/abstract_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include "execution/executor_context.h"
#include "storage/table/tuple.h"

namespace bustub {
class ExecutorContext;
/**
 * AbstractExecutor 实现了 Volcano 模型中的逐元组迭代器模式。
 * 这是 Bustub 执行引擎中所有执行器的基类，定义了所有执行器必须支持的最小接口。
 */

class AbstractExecutor {
 public:
  /**
   * Construct a new AbstractExecutor instance.
   * @param exec_ctx the executor context that the executor runs with
   */
  explicit AbstractExecutor(ExecutorContext *exec_ctx) : exec_ctx_{exec_ctx} {}

  /** Virtual destructor. */
  virtual ~AbstractExecutor() = default;

  /**
   * Initialize the executor.
   * @warning This function must be called before Next() is called!
   */
  virtual void Init() = 0;

  /**
   * 从此执行器获取下一个元组。
   * @param[out] tuple 此执行器生成的下一个元组
   * @param[out] rid 此执行器生成的下一个元组的 RID（记录 ID）
   * @return 如果生成了元组，则返回 `true`；如果没有更多元组，则返回 `false`
   */
  virtual auto Next(Tuple *tuple, RID *rid) -> bool = 0;

  /** @return The schema of the tuples that this executor produces */
  virtual auto GetOutputSchema() const -> const Schema & = 0;

  /** @return The executor context in which this executor runs */
  auto GetExecutorContext() -> ExecutorContext * { return exec_ctx_; }

 protected:
  /** The executor context in which the executor runs */
  ExecutorContext *exec_ctx_;
};
}  // namespace bustub
