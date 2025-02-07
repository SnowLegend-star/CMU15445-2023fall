//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// tuple.h
//
// Identification: src/include/storage/table/tuple.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>

#include "catalog/schema.h"
#include "common/config.h"
#include "common/rid.h"
#include "type/value.h"

namespace bustub {

using timestamp_t = int64_t;
const timestamp_t INVALID_TS = -1;

static constexpr size_t TUPLE_META_SIZE = 16;

struct TupleMeta {
  /** the ts / txn_id of this tuple. In project 3, simply set it to 0. */
  timestamp_t ts_;
  /** marks whether this tuple is marked removed from table heap. */
  bool is_deleted_;

  friend auto operator==(const TupleMeta &a, const TupleMeta &b) {
    return a.ts_ == b.ts_ && a.is_deleted_ == b.is_deleted_;
  }

  friend auto operator!=(const TupleMeta &a, const TupleMeta &b) { return !(a == b); }
};

static_assert(sizeof(TupleMeta) == TUPLE_META_SIZE);

/**
 * Tuple format:
 * ---------------------------------------------------------------------
 * | FIXED-SIZE or VARIED-SIZED OFFSET | PAYLOAD OF VARIED-SIZED FIELD |
 * ---------------------------------------------------------------------
 */

/**
 * 元组格式：
 * ---------------------------------------------------------------------
 * | 固定大小或变动大小偏移量 | 变动大小字段的有效载荷 |
 * ---------------------------------------------------------------------
 */
class Tuple {
  friend class TablePage;
  friend class TableHeap;
  friend class TableIterator;

 public:
  // 默认构造函数（用于创建一个虚拟元组）
  Tuple() = default;

  // 用于表堆元组的构造函数
  explicit Tuple(RID rid) : rid_(rid) {}

  static auto Empty() -> Tuple { return Tuple{RID{INVALID_PAGE_ID, 0}}; }

  // 根据输入值创建一个新的元组
  Tuple(std::vector<Value> values, const Schema *schema);

  Tuple(const Tuple &other) = default;

  // 移动构造函数
  Tuple(Tuple &&other) noexcept = default;

  // 赋值操作符，深拷贝
  auto operator=(const Tuple &other) -> Tuple & = default;

  // 移动赋值操作符
  auto operator=(Tuple &&other) noexcept -> Tuple & = default;

  // 序列化元组数据
  void SerializeTo(char *storage) const;

  // 反序列化元组数据（深拷贝）
  void DeserializeFrom(const char *storage);

  // 返回当前元组的RID
  inline auto GetRid() const -> RID { return rid_; }

  // 设置当前元组的RID
  inline auto SetRid(RID rid) { rid_ = rid; }

  // 获取该元组在表的备份存储中的地址
  inline auto GetData() const -> const char * { return data_.data(); }

  // 获取元组的长度，包括varchar的长度
  inline auto GetLength() const -> uint32_t { return data_.size(); }

  // 获取指定列的值（常量）
  // 检查模式，以确定如何返回值。
  auto GetValue(const Schema *schema, uint32_t column_idx) const -> Value;

  // 根据模式和属性生成一个键元组
  auto KeyFromTuple(const Schema &schema, const Schema &key_schema, const std::vector<uint32_t> &key_attrs) -> Tuple;

  // 列值是否为null？
  inline auto IsNull(const Schema *schema, uint32_t column_idx) const -> bool {
    Value value = GetValue(schema, column_idx);
    return value.IsNull();
  }

  auto ToString(const Schema *schema) const -> std::string;

  friend inline auto IsTupleContentEqual(const Tuple &a, const Tuple &b) { return a.data_ == b.data_; }

 private:
  // 获取特定列的起始存储地址
  auto GetDataPtr(const Schema *schema, uint32_t column_idx) const -> const char *;

  RID rid_{};  // 如果指向表堆，RID是有效的
  std::vector<char> data_;
};

}  // namespace bustub
