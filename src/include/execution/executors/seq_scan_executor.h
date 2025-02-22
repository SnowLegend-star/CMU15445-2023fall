//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.h
//
// Identification: src/include/execution/executors/seq_scan_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <vector>

#include "catalog/catalog.h"
#include "common/rid.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/table_heap.h"
#include "storage/table/tuple.h"
#include "concurrency/transaction_manager.h"

namespace bustub {

/**
 * SeqScanExecutor 执行器执行顺序表扫描。
 */
class SeqScanExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 SeqScanExecutor 实例。
   * @param exec_ctx 执行器上下文
   * @param plan 要执行的顺序扫描计划
   */
  SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan);

  /** 初始化顺序扫描 */
  void Init() override;

  /**
   * 生成顺序扫描中的下一个元组。
   * @param[out] tuple 顺序扫描产生的下一个元组
   * @param[out] rid 顺序扫描产生的下一个元组的 RID
   * @return 如果生成了元组，则返回 `true`；如果没有更多元组，则返回 `false`
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return 顺序扫描的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** 要执行的顺序扫描计划节点 */
  const SeqScanPlanNode *plan_;
  // TableInfo *table_info_;
  // TableHeap *table_heap_;
  std::vector<RID> rids_;
  std::unique_ptr<TableIterator> iter_;
};
}  // namespace bustub
