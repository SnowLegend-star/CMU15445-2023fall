//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// table_iterator.h
//
// Identification: src/include/storage/table/table_iterator.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <memory>
#include <utility>

#include "common/macros.h"
#include "common/rid.h"
#include "concurrency/transaction.h"
#include "storage/table/tuple.h"

namespace bustub {

class TableHeap;

/**
 * TableIterator enables the sequential scan of a TableHeap.
 */
class TableIterator {
  friend class Cursor;

 public:
  DISALLOW_COPY(TableIterator);
  // table_heap：指向 TableHeap 的指针，表明该迭代器操作的表。
  // rid：开始扫描的起始 RID（记录 ID），表示从哪一行（元组）开始遍历。
  // stop_at_rid：指定一个结束的 RID，当扫描到该记录时，迭代器将停止，防止出现死循环（尤其是在项目 4 中，更新操作可能会涉及删除和插入）。
  TableIterator(TableHeap *table_heap, RID rid, RID stop_at_rid);
  TableIterator(TableIterator &&) = default;

  ~TableIterator() = default;

  auto GetTuple() -> std::pair<TupleMeta, Tuple>;

  auto GetRID() -> RID;

  auto IsEnd() -> bool;

  auto operator++() -> TableIterator &;

 private:
  TableHeap *table_heap_;
  RID rid_;

  // When creating table iterator, we will record the maximum RID that we should scan.
  // Otherwise we will have dead loops when updating while scanning. (In project 4, update should be implemented as
  // deletion + insertion.)

  // stop_at_rid_ 是用于防止死循环的一个机制。
  // 假设在某些操作中，更新可能涉及删除和插入（例如项目 4 中的操作），如果没有停止条件，迭代器可能会陷入死循环（例如扫描到一个被删除的元组后又插入回表中）。
  // 通过设置 stop_at_rid_，可以确保迭代器在达到指定的结束位置后停止。
  RID stop_at_rid_;
};

}  // namespace bustub
