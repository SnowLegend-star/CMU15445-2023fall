//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// execution_engine.h
//
// Identification: src/include/execution/execution_engine.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"
#include "execution/executor_context.h"
#include "execution/executor_factory.h"
#include "execution/executors/init_check_executor.h"
#include "execution/plans/abstract_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * The ExecutionEngine class executes query plans.
 */
class ExecutionEngine {
 public:
  /**
   * 构造一个新的 ExecutionEngine 实例。
   * @param bpm 执行引擎使用的缓冲池管理器
   * @param txn_mgr 执行引擎使用的事务管理器
   * @param catalog 执行引擎使用的目录
   */
  ExecutionEngine(BufferPoolManager *bpm, TransactionManager *txn_mgr, Catalog *catalog)
      : bpm_{bpm}, txn_mgr_{txn_mgr}, catalog_{catalog} {}

  DISALLOW_COPY_AND_MOVE(ExecutionEngine);

  /**
   * 执行一个查询计划。
   * @param plan 要执行的查询计划
   * @param result_set 执行计划后生成的元组集合
   * @param txn 查询执行时的事务上下文
   * @param exec_ctx 查询执行时的执行上下文
   * @return 如果查询计划执行成功，返回 `true`；否则返回 `false`
   */
  // NOLINTNEXTLINE
  auto Execute(const AbstractPlanNodeRef &plan, std::vector<Tuple> *result_set, Transaction *txn,
               ExecutorContext *exec_ctx) -> bool {
    BUSTUB_ASSERT((txn == exec_ctx->GetTransaction()), "Broken Invariant");

    // 为抽象计划节点构造执行器
    auto executor = ExecutorFactory::CreateExecutor(exec_ctx, plan);

    // 初始化执行器
    auto executor_succeeded = true;

    try {
      executor->Init();
      PollExecutor(executor.get(), plan, result_set);
      PerformChecks(exec_ctx);
    } catch (const ExecutionException &ex) {
      executor_succeeded = false;
      if (result_set != nullptr) {
        result_set->clear();
      }
    }

    return executor_succeeded;
  }

  void PerformChecks(ExecutorContext *exec_ctx) {
    for (const auto &[left_executor, right_executor] : exec_ctx->GetNLJCheckExecutorSet()) {
      auto casted_left_executor = dynamic_cast<const InitCheckExecutor *>(left_executor);
      auto casted_right_executor = dynamic_cast<const InitCheckExecutor *>(right_executor);
      BUSTUB_ASSERT(casted_right_executor->GetInitCount() + 1 >= casted_left_executor->GetNextCount(),
                    "nlj check failed, are you initialising the right executor every time when there is a left tuple? "
                    "(off-by-one is okay)");
    }
  }

 private:
  /**
   * 一直轮询执行器直到耗尽，或者异常被抛出。
   * @param executor 根执行器
   * @param plan 要执行的计划
   * @param result_set 元组结果集
   */
  static void PollExecutor(AbstractExecutor *executor, const AbstractPlanNodeRef &plan,
                           std::vector<Tuple> *result_set) {
    RID rid{};
    Tuple tuple{};
    while (executor->Next(&tuple, &rid)) {
      if (result_set != nullptr) {
        result_set->push_back(tuple);
      }
    }
  }

  [[maybe_unused]] BufferPoolManager *bpm_;
  [[maybe_unused]] TransactionManager *txn_mgr_;
  [[maybe_unused]] Catalog *catalog_;
};

}  // namespace bustub
