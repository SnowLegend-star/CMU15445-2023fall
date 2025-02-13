//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// executor_context.h
//
// Identification: src/include/execution/executor_context.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <deque>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "execution/check_options.h"
#include "execution/executors/abstract_executor.h"
#include "storage/page/tmp_tuple_page.h"

namespace bustub {
class AbstractExecutor;
/**
 * ExecutorContext 存储了执行执行器所需的所有上下文。
 */
class ExecutorContext {
 public:
  /**
   * 为执行查询的事务创建一个 ExecutorContext。
   * @param transaction 执行查询的事务
   * @param catalog 执行器使用的目录
   * @param bpm 执行器使用的缓冲池管理器
   * @param txn_mgr 执行器使用的事务管理器
   * @param lock_mgr 执行器使用的锁管理器
   */
  ExecutorContext(Transaction *transaction, Catalog *catalog, BufferPoolManager *bpm, TransactionManager *txn_mgr,
                  LockManager *lock_mgr, bool is_delete)
      : transaction_(transaction),
        catalog_{catalog},
        bpm_{bpm},
        txn_mgr_(txn_mgr),
        lock_mgr_(lock_mgr),
        is_delete_(is_delete) {
    nlj_check_exec_set_ = std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>>(
        std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>>{});  // 初始化 NLJ 检查执行器集合
    check_options_ = std::make_shared<CheckOptions>();                     // 初始化检查选项
  }

  ~ExecutorContext() = default;

  DISALLOW_COPY_AND_MOVE(ExecutorContext);

  /** @return 正在运行的事务 */
  auto GetTransaction() const -> Transaction * { return transaction_; }

  /** @return 数据库目录 */
  auto GetCatalog() -> Catalog * { return catalog_; }

  /** @return 缓冲池管理器 */
  auto GetBufferPoolManager() -> BufferPoolManager * { return bpm_; }

  /** @return 日志管理器 - 目前不用担心 */
  auto GetLogManager() -> LogManager * { return nullptr; }

  /** @return 锁管理器 */
  auto GetLockManager() -> LockManager * { return lock_mgr_; }

  /** @return 事务管理器 */
  auto GetTransactionManager() -> TransactionManager * { return txn_mgr_; }

  /** @return NLJ 检查执行器集合 */
  auto GetNLJCheckExecutorSet() -> std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>> & {
    return nlj_check_exec_set_;
  }

  /** @return 检查选项 */
  auto GetCheckOptions() -> std::shared_ptr<CheckOptions> { return check_options_; }

  void AddCheckExecutor(AbstractExecutor *left_exec, AbstractExecutor *right_exec) {
    nlj_check_exec_set_.emplace_back(left_exec, right_exec);  // 添加 NLJ 检查执行器
  }

  void InitCheckOptions(std::shared_ptr<CheckOptions> &&check_options) {
    BUSTUB_ASSERT(check_options, "nullptr");
    check_options_ = std::move(check_options);  // 初始化检查选项
  }

  /** 从 2023 年秋季开始，不应使用此函数。 */
  auto IsDelete() const -> bool { return is_delete_; }

 private:
  /** 与此执行器上下文关联的事务上下文 */
  Transaction *transaction_;
  /** 与此执行器上下文关联的数据库目录 */
  Catalog *catalog_;
  /** 与此执行器上下文关联的缓冲池管理器 */
  BufferPoolManager *bpm_;
  /** 与此执行器上下文关联的事务管理器 */
  TransactionManager *txn_mgr_;
  /** 与此执行器上下文关联的锁管理器 */
  LockManager *lock_mgr_;
  /** 与此执行器上下文关联的 NLJ 检查执行器集合 */
  std::deque<std::pair<AbstractExecutor *, AbstractExecutor *>> nlj_check_exec_set_;
  /** 与此执行器上下文关联的检查选项集合 */
  std::shared_ptr<CheckOptions> check_options_;
  bool is_delete_;  // 标记是否为删除操作
};

}  // namespace bustub
