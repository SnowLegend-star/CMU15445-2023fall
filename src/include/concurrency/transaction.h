//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// transaction.h
//
// Identification: src/include/concurrency/transaction.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <fmt/format.h>
#include <atomic>
#include <bitset>
#include <cstddef>
#include <deque>
#include <limits>
#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/logger.h"
#include "execution/expressions/abstract_expression.h"
#include "storage/page/page.h"
#include "storage/table/tuple.h"

namespace bustub {

class TransactionManager;

/**
  * 事务状态。
  */
enum class TransactionState { RUNNING = 0, TAINTED, COMMITTED = 100, ABORTED };

/**
  * 事务隔离级别。READ_UNCOMMITTED 在2023年秋季的项目3/4中将不使用。
  */
enum class IsolationLevel { READ_UNCOMMITTED, SNAPSHOT_ISOLATION, SERIALIZABLE };

class TableHeap;
class Catalog;
using table_oid_t = uint32_t;
using index_oid_t = uint32_t;

/** 表示指向此元组的前一个版本的链接 */
struct UndoLink {
  /* 前一个版本所在的事务 */
  txn_id_t prev_txn_{INVALID_TXN_ID};
  /* 前一个版本在`prev_txn_`中的日志索引 */
  int prev_log_idx_{0};

  friend auto operator==(const UndoLink &a, const UndoLink &b) {
    return a.prev_txn_ == b.prev_txn_ && a.prev_log_idx_ == b.prev_log_idx_;
  }

  friend auto operator!=(const UndoLink &a, const UndoLink &b) { return !(a == b); }

  /* 检查回滚链接是否有效 */
  auto IsValid() const -> bool { return prev_txn_ != INVALID_TXN_ID; }
};

struct UndoLog {
  /* 此日志是否为删除标记 */
  bool is_deleted_;
  /* 被此回滚日志修改的字段 */
  std::vector<bool> modified_fields_;
  /* 被修改的字段 */
  Tuple tuple_;
  /* 此回滚日志的时间戳 */
  timestamp_t ts_{INVALID_TS};
  /* 回滚日志的前一个版本 */
  UndoLink prev_version_{};
};

/**
  * 事务类，跟踪与事务相关的信息。
  */
class Transaction {
  public:
  explicit Transaction(txn_id_t txn_id, IsolationLevel isolation_level = IsolationLevel::SNAPSHOT_ISOLATION)
      : isolation_level_(isolation_level), thread_id_(std::this_thread::get_id()), txn_id_(txn_id) {}

  ~Transaction() = default;

  DISALLOW_COPY(Transaction);

  /** @return 正在执行此事务的线程的ID */
  inline auto GetThreadId() const -> std::thread::id { return thread_id_; }

  /** @return 此事务的ID */
  inline auto GetTransactionId() const -> txn_id_t { return txn_id_; }

  /** @return 此事务的ID，去掉最高位。除非用于调试，否则永远不要使用/存储此值。 */
  inline auto GetTransactionIdHumanReadable() const -> txn_id_t { return txn_id_ ^ TXN_START_ID; }

  /** @return 此事务的临时时间戳 */
  inline auto GetTransactionTempTs() const -> timestamp_t { return txn_id_; }

  /** @return 此事务的隔离级别 */
  inline auto GetIsolationLevel() const -> IsolationLevel { return isolation_level_; }

  /** @return 此事务的状态 */
  inline auto GetTransactionState() const -> TransactionState { return state_; }

  /** @return 此事务的读取时间戳 */
  inline auto GetReadTs() const -> timestamp_t { return read_ts_; }

  /** @return 此事务的提交时间戳 */
  inline auto GetCommitTs() const -> timestamp_t { return commit_ts_; }

  /** 修改现有的回滚日志 */
  inline auto ModifyUndoLog(int log_idx, UndoLog new_log) {
    std::scoped_lock<std::mutex> lck(latch_);
    undo_logs_[log_idx] = std::move(new_log);
  }

  /** @return 在此事务中的回滚日志索引 */
  inline auto AppendUndoLog(UndoLog log) -> UndoLink {
    std::scoped_lock<std::mutex> lck(latch_);
    undo_logs_.emplace_back(std::move(log));
    return {txn_id_, static_cast<int>(undo_logs_.size() - 1)};
  }

  inline auto AppendWriteSet(table_oid_t t, RID rid) {
    std::scoped_lock<std::mutex> lck(latch_);
    write_set_[t].insert(rid);
  }

  inline auto GetWriteSets() -> const std::unordered_map<table_oid_t, std::unordered_set<RID>> & { return write_set_; }

  inline auto AppendScanPredicate(table_oid_t t, const AbstractExpressionRef &predicate) {
    std::scoped_lock<std::mutex> lck(latch_);
    scan_predicates_[t].emplace_back(predicate);
  }

  inline auto GetScanPredicates() -> const std::unordered_map<table_oid_t, std::vector<AbstractExpressionRef>> & {
    return scan_predicates_;
  }

  inline auto GetUndoLog(size_t log_id) -> UndoLog {
    std::scoped_lock<std::mutex> lck(latch_);
    return undo_logs_[log_id];
  }

  inline auto GetUndoLogNum() -> size_t {
    std::scoped_lock<std::mutex> lck(latch_);
    return undo_logs_.size();
  }

  /** 在排行榜基准测试中为在线垃圾回收使用此函数。对于停止世界垃圾回收，只需从txn_map中移除事务。 */
  inline auto ClearUndoLog() -> size_t {
    std::scoped_lock<std::mutex> lck(latch_);
    return undo_logs_.size();
  }

  void SetTainted();

  private:
  friend class TransactionManager;

  // 以下字段应仅由事务管理器（持有事务管理器锁）更改。

  /** 此事务的状态 */
  std::atomic<TransactionState> state_{TransactionState::RUNNING};

  /** 读取时间戳 */
  std::atomic<timestamp_t> read_ts_{0};

  /** 提交时间戳 */
  std::atomic<timestamp_t> commit_ts_{INVALID_TS};

  /** 该事务的锁，用于访问事务级回滚日志，保护以下所有字段。 */
  std::mutex latch_;

  /**
    * @brief 存储回滚日志。其他回滚日志/表堆将存储(txn_id, 索引)对，因此
    * 你应该仅向此向量追加内容或就地更新，而不是删除任何内容。
    */
  std::vector<UndoLog> undo_logs_;

  /** 存储写入元组的RID */
  std::unordered_map<table_oid_t, std::unordered_set<RID>> write_set_;
  /** 存储所有扫描谓词 */
  std::unordered_map<table_oid_t, std::vector<AbstractExpressionRef>> scan_predicates_;

  // 以下字段在创建事务时设置，并且永远不会更改。

  /** 事务的隔离级别 */
  const IsolationLevel isolation_level_;

  /** 事务开始时的线程ID */
  const std::thread::id thread_id_;

  /** 此事务的ID */
  const txn_id_t txn_id_;
};

}  // namespace bustub

template <>
struct fmt::formatter<bustub::IsolationLevel> : formatter<std::string_view> {
  // parse 从formatter<string_view>继承。
  template <typename FormatContext>
  auto format(bustub::IsolationLevel x, FormatContext &ctx) const {
    using bustub::IsolationLevel;
    string_view name = "unknown";
    switch (x) {
      case IsolationLevel::READ_UNCOMMITTED:
        name = "READ_UNCOMMITTED";
        break;
      case IsolationLevel::SNAPSHOT_ISOLATION:
        name = "SNAPSHOT_ISOLATION";
        break;
      case IsolationLevel::SERIALIZABLE:
        name = "SERIALIZABLE";
        break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};

template <>
struct fmt::formatter<bustub::TransactionState> : formatter<std::string_view> {
  // parse 从formatter<string_view>继承。
  template <typename FormatContext>
  auto format(bustub::TransactionState x, FormatContext &ctx) const {
    using bustub::TransactionState;
    string_view name = "unknown";
    switch (x) {
      case TransactionState::RUNNING:
        name = "RUNNING";
        break;
      case TransactionState::ABORTED:
        name = "ABORTED";
        break;
      case TransactionState::COMMITTED:
        name = "COMMITTED";
        break;
      case TransactionState::TAINTED:
        name = "TAINTED";
        break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};  