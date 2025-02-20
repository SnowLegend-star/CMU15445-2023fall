//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// transaction_manager.h
//
// Identification: src/include/concurrency/transaction_manager.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>  // NOLINT
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "catalog/schema.h"
#include "common/config.h"
#include "concurrency/transaction.h"
#include "concurrency/watermark.h"
#include "recovery/log_manager.h"
#include "storage/table/tuple.h"

namespace bustub {

/// 版本链中的第一个回滚链接，将表堆元组与回滚日志连接起来。
struct VersionUndoLink {
  /** 版本链中的下一个版本。 */
  UndoLink prev_;
  /** 事务是否正在修改版本链接。2023年秋季：在任务4.2之前，你不需要读取/写入此字段。 */
  bool in_progress_{false};

  friend auto operator==(const VersionUndoLink &a, const VersionUndoLink &b) {
    return a.prev_ == b.prev_ && a.in_progress_ == b.in_progress_;
  }

  friend auto operator!=(const VersionUndoLink &a, const VersionUndoLink &b) { return !(a == b); }

  inline static auto FromOptionalUndoLink(std::optional<UndoLink> undo_link) -> std::optional<VersionUndoLink> {
    if (undo_link.has_value()) {
      return VersionUndoLink{*undo_link};
    }
    return std::nullopt;
  }
};

/**
 * TransactionManager 跟踪系统中所有运行的事务。
 */
class TransactionManager {
 public:
  TransactionManager() = default;
  ~TransactionManager() = default;

  /**
   * 开始一个新事务。
   * @param isolation_level 事务的可选隔离级别。
   * @return 初始化的事务
   */
  auto Begin(IsolationLevel isolation_level = IsolationLevel::SNAPSHOT_ISOLATION) -> Transaction *;

  /**
   * 提交一个事务。
   * @param txn 要提交的事务，事务会由事务管理器管理，因此不需要手动删除它
   */
  auto Commit(Transaction *txn) -> bool;

  /**
   * 中止一个事务
   * @param txn 要中止的事务，事务会由事务管理器管理，因此不需要手动删除它
   */
  void Abort(Transaction *txn);

  /**
   * @brief 在任务4.2之前使用此函数。更新将表堆元组与第一个回滚日志连接的回滚链接。
   * 在更新之前，将调用`check`函数以确保有效性。
   */
  auto UpdateUndoLink(RID rid, std::optional<UndoLink> prev_link,
                      std::function<bool(std::optional<UndoLink>)> &&check = nullptr) -> bool;

  /**
   * @brief 在任务4.2之后使用此函数。更新将表堆元组与第一个回滚日志连接的版本链接。
   * 在更新之前，将调用`check`函数以确保有效性。
   */
  auto UpdateVersionLink(RID rid, std::optional<VersionUndoLink> prev_version,
                         std::function<bool(std::optional<VersionUndoLink>)> &&check = nullptr) -> bool;

  /** @brief 获取表堆元组的第一个回滚日志。任务4.2之前使用 */
  auto GetUndoLink(RID rid) -> std::optional<UndoLink>;

  /** @brief 获取表堆元组的第一个回滚日志。任务4.2之后使用 */
  auto GetVersionLink(RID rid) -> std::optional<VersionUndoLink>;

  /** @brief 访问事务回滚日志缓冲区并获取回滚日志。如果事务不存在，返回nullopt。如果索引超出范围，仍会抛出异常。 */
  auto GetUndoLogOptional(UndoLink link) -> std::optional<UndoLog>;

  /** @brief 访问事务回滚日志缓冲区并获取回滚日志。除非访问当前事务缓冲区，否则你应该总是调用此函数来获取回滚日志，而不是手动获取事务共享指针并访问缓冲区。 */
  auto GetUndoLog(UndoLink link) -> UndoLog;

  /** @brief 获取系统中最低的读取时间戳。 */
  auto GetWatermark() -> timestamp_t { return running_txns_.GetWatermark(); }

  /** @brief 停止世界垃圾回收。当所有事务不再访问表堆时，会调用此函数。 */
  void GarbageCollection();

  /** 保护事务映射 */
  std::shared_mutex txn_map_mutex_;
  /** 所有事务，无论是正在运行还是已提交 */
  std::unordered_map<txn_id_t, std::shared_ptr<Transaction>> txn_map_;

  struct PageVersionInfo {
    /** 保护映射 */
    std::shared_mutex mutex_;
    /** 存储所有槽的上一个版本信息。注意：不要使用`[x]`来访问，因为这样会创建新的元素，即使它不存在。应该使用`find`来访问。 */
    std::unordered_map<slot_offset_t, VersionUndoLink> prev_version_;
  };

  /** 保护版本信息 */
  std::shared_mutex version_info_mutex_;
  /** 存储表堆中每个元组的前一个版本。不要直接访问此字段。使用`transaction_manager_impl.cpp`中的辅助函数。 */
  std::unordered_map<page_id_t, std::shared_ptr<PageVersionInfo>> version_info_;

  /** 存储所有正在运行的事务的读时间戳，以便促进垃圾回收。 */
  Watermark running_txns_{0};

  /** 只允许一个事务在同一时间提交 */
  std::mutex commit_mutex_;
  /** 最后提交的时间戳。 */
  std::atomic<timestamp_t> last_commit_ts_{0};

  /** 目录 */
  Catalog *catalog_;

  std::atomic<txn_id_t> next_txn_id_{TXN_START_ID};

 private:
  /** @brief 验证事务是否满足可串行化。我们不会测试此函数，你可以根据需要更改或删除它。 */
  auto VerifyTxn(Transaction *txn) -> bool;
};

}  // namespace bustub
