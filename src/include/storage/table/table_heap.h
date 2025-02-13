//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// table_heap.h
//
// Identification: src/include/storage/table/table_heap.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <mutex>  // NOLINT
#include <optional>
#include <utility>

#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "concurrency/lock_manager.h"
#include "concurrency/transaction.h"
#include "recovery/log_manager.h"
#include "storage/page/page_guard.h"
#include "storage/page/table_page.h"
#include "storage/table/table_iterator.h"
#include "storage/table/tuple.h"

namespace bustub {

class TablePage;

/**
 * TableHeap 表示磁盘上的物理表。
 * 这只是一个双向链表的页面。
 */
class TableHeap {
  friend class TableIterator;

 public:
  ~TableHeap() = default;

  /**
   * 在没有事务的情况下创建一个表堆。（打开表）
   * @param buffer_pool_manager 缓冲池管理器
   * @param first_page_id 表的第一页的 ID
   */
  explicit TableHeap(BufferPoolManager *bpm);

  /**
   * 将一个元组插入到表中。如果元组太大（>= 页面大小），则返回 std::nullopt。
   * @param meta 元组的元数据
   * @param tuple 要插入的元组
   * @return 插入的元组的 RID
   */
  auto InsertTuple(const TupleMeta &meta, const Tuple &tuple, LockManager *lock_mgr = nullptr,
                   Transaction *txn = nullptr, table_oid_t oid = 0) -> std::optional<RID>;

  /**
   * Update the meta of a tuple.
   * @param meta new tuple meta
   * @param rid the rid of the inserted tuple
   */
  void UpdateTupleMeta(const TupleMeta &meta, RID rid);

  /**
   * Read a tuple from the table.
   * @param rid rid of the tuple to read
   * @return the meta and tuple
   */
  auto GetTuple(RID rid) -> std::pair<TupleMeta, Tuple>;

  /**
   * 从表中读取一个元组的元数据。注意：如果你想同时获取元组和元数据，请使用 `GetTuple` 以确保原子性。
   * @param rid 要读取的元组的 rid
   * @return 元数据
   */
  auto GetTupleMeta(RID rid) -> TupleMeta;

  /**
   * 返回该表的迭代器。当这个迭代器被创建时，它将记录表中当前的最后一个元组，迭代器将在该点停止，
   * 以避免万圣节问题。通常你会在项目 3 中使用这个函数。如果你已经实现了项目 4 的更新执行器作为
   * 管道中断，你可以使用 `MakeEagerIterator` 来测试更新执行器是否正确实现。
   * 如果一切实现正确，这个函数和项目 4 中的 `MakeEagerIterator` 应该没有区别。
   */
  auto MakeIterator() -> TableIterator;

  /** @return the iterator of this table. The iterator will stop at the last tuple at the time of iterating. */
  auto MakeEagerIterator() -> TableIterator;

  /** @return the id of the first page of this table */
  inline auto GetFirstPageId() const -> page_id_t { return first_page_id_; }

  /**
   * 就地更新一个元组。这个函数不应在项目 3 中使用。请在项目 3 中将更新执行器实现为删除和插入。
   * 你将在项目 4 中使用这个函数。
   * @param meta 新的元组元数据
   * @param tuple 新的元组
   * @param rid 要更新的元组的 rid
   * @param check 在实际更新之前要运行的检查
   */
  auto UpdateTupleInPlace(const TupleMeta &meta, const Tuple &tuple, RID rid,
                          std::function<bool(const TupleMeta &meta, const Tuple &table, RID rid)> &&check = nullptr)
      -> bool;

  /** For binder tests */
  static auto CreateEmptyHeap(bool create_table_heap = false) -> std::unique_ptr<TableHeap> {
    // The input parameter should be false in order to generate a empty heap
    assert(!create_table_heap);
    return std::unique_ptr<TableHeap>(new TableHeap(create_table_heap));
  }

  // 以下函数仅在你想实现撤销时删除版本链中的撤销日志时有用。
  // 如果你不确定这些函数的作用，请不要使用它们。
  //
  // 如果你决定使用以下函数，千万不要使用像 `GetTuple` 这样的普通函数。
  // 在一个线程中对同一对象使用两个读取锁可能会导致死锁。

  auto AcquireTablePageReadLock(RID rid) -> ReadPageGuard;

  auto AcquireTablePageWriteLock(RID rid) -> WritePageGuard;

  void UpdateTupleInPlaceWithLockAcquired(const TupleMeta &meta, const Tuple &tuple, RID rid, TablePage *page);

  auto GetTupleWithLockAcquired(RID rid, const TablePage *page) -> std::pair<TupleMeta, Tuple>;

  auto GetTupleMetaWithLockAcquired(RID rid, const TablePage *page) -> TupleMeta;

 private:
  /** Used for binder tests */
  explicit TableHeap(bool create_table_heap = false);

  BufferPoolManager *bpm_;
  page_id_t first_page_id_{INVALID_PAGE_ID};

  std::mutex latch_;
  page_id_t last_page_id_{INVALID_PAGE_ID}; /* protected by latch_ */
};

}  // namespace bustub
