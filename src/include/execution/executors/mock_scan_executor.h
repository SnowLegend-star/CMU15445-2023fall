//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// mock_scan_executor.h
//
// Identification: src/include/execution/executors/mock_scan_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/mock_scan_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

extern const char *mock_table_list[];
auto GetMockTableSchemaOf(const std::string &table) -> Schema;

/**
 * MockScanExecutor 执行一个用于测试的顺序表扫描。
 */
class MockScanExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 MockScanExecutor 实例。
   * @param exec_ctx 执行上下文
   * @param plan 要执行的模拟扫描计划
   */
  MockScanExecutor(ExecutorContext *exec_ctx, const MockScanPlanNode *plan);

  /** 初始化模拟扫描。 */
  void Init() override;

  /**
   * 输出顺序扫描的下一个元组。
   * @param[out] tuple 扫描生成的下一个元组
   * @param[out] rid 扫描生成的下一个元组的 RID
   * @return 如果生成了元组，返回 `true`；如果没有更多元组，返回 `false`
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return 顺序扫描的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** @return 根据输出模式生成一个虚拟元组 */
  auto MakeDummyTuple() const -> Tuple;

  /** @return 一个虚拟的 RID 值 */
  static auto MakeDummyRID() -> RID;

  /** MockScanExecutor::Next() 在扫描未完成时返回 `true` */
  constexpr static const bool EXECUTOR_ACTIVE{true};

  /** MockScanExecutor::Next() 在扫描完成时返回 `false` */
  constexpr static const bool EXECUTOR_EXHAUSTED{false};

  /** 扫描的计划节点 */
  const MockScanPlanNode *plan_;

  /** 当前模拟扫描的游标 */
  std::size_t cursor_{0};

  /** 表的函数 */
  std::function<Tuple(std::size_t)> func_;

  /** 模拟表的大小 */
  std::size_t size_;

  /** 打乱顺序后的输出 */
  std::vector<size_t> shuffled_idx_;
};

}  // namespace bustub
