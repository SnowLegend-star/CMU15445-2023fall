//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// catalog.h
//
// Identification: src/include/catalog/catalog.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "catalog/schema.h"
#include "container/hash/hash_function.h"
#include "storage/index/b_plus_tree_index.h"
#include "storage/index/extendible_hash_table_index.h"
#include "storage/index/index.h"
#include "storage/table/table_heap.h"

namespace bustub {

/**
 * Typedefs
 */
using table_oid_t = uint32_t;
using column_oid_t = uint32_t;
using index_oid_t = uint32_t;

enum class IndexType { BPlusTreeIndex, HashTableIndex };

/**
 * TableInfo 类维护表的元数据。
 */
struct TableInfo {
  /**
   * 构造一个新的 TableInfo 实例。
   * @param schema 表的模式
   * @param name 表的名称
   * @param table 表堆的拥有指针
   * @param oid 表的唯一 OID
   */
  TableInfo(Schema schema, std::string name, std::unique_ptr<TableHeap> &&table, table_oid_t oid)
      : schema_{std::move(schema)}, name_{std::move(name)}, table_{std::move(table)}, oid_{oid} {}
  
  /** 表的模式 */
  Schema schema_;
  
  /** 表的名称 */
  const std::string name_;
  
  /** 表堆的拥有指针 */
  std::unique_ptr<TableHeap> table_;
  
  /** 表的 OID */
  const table_oid_t oid_;
};

/**
 * IndexInfo 类维护索引的元数据。
 */
struct IndexInfo {
  /**
   * 构造一个新的 IndexInfo 实例。
   * @param key_schema 索引键的模式
   * @param name 索引的名称
   * @param index 索引的拥有指针
   * @param index_oid 索引的唯一 OID
   * @param table_name 创建索引的表名
   * @param key_size 索引键的大小，单位为字节
   */
  IndexInfo(Schema key_schema, std::string name, std::unique_ptr<Index> &&index, index_oid_t index_oid,
            std::string table_name, size_t key_size, bool is_primary_key)
      : key_schema_{std::move(key_schema)},
        name_{std::move(name)},
        index_{std::move(index)},
        index_oid_{index_oid},
        table_name_{std::move(table_name)},
        key_size_{key_size},
        is_primary_key_{is_primary_key} {}

  /** 索引键的模式 */
  Schema key_schema_;
  
  /** 索引的名称 */
  std::string name_;
  
  /** 索引的拥有指针 */
  std::unique_ptr<Index> index_;
  
  /** 索引的唯一 OID */
  index_oid_t index_oid_;
  
  /** 创建索引的表名 */
  std::string table_name_;
  
  /** 索引键的大小，单位为字节 */
  const size_t key_size_;
  
  /** 是否为主键索引 */
  bool is_primary_key_;
  
  /** 索引类型 */
  [[maybe_unused]] IndexType index_type_{IndexType::BPlusTreeIndex};
};


/**
 * Catalog 是一个非持久化的目录，专为 DBMS 执行引擎中的执行器使用。
 * 它处理表的创建、表的查找、索引的创建和索引的查找。
 */
class Catalog {
 public:
  /** 表示操作返回的 `TableInfo*` 失败 */
  static constexpr TableInfo *NULL_TABLE_INFO{nullptr};

  /** 表示操作返回的 `IndexInfo*` 失败 */
  static constexpr IndexInfo *NULL_INDEX_INFO{nullptr};

  /**
   * 构造一个新的 Catalog 实例。
   * @param bpm 用于支撑由该目录创建的表的缓冲池管理器
   * @param lock_manager 系统使用的锁管理器
   * @param log_manager 系统使用的日志管理器
   */
  Catalog(BufferPoolManager *bpm, LockManager *lock_manager, LogManager *log_manager)
      : bpm_{bpm}, lock_manager_{lock_manager}, log_manager_{log_manager} {}

  /**
   * 创建一个新表并返回其元数据。
   * @param txn 创建表的事务
   * @param table_name 新表的名称，注意所有以 `__` 开头的表名是系统保留的
   * @param schema 新表的模式
   * @param create_table_heap 是否为新表创建一个表堆
   * @return 表的元数据（非拥有者指针）
   */
  auto CreateTable(Transaction *txn, const std::string &table_name, const Schema &schema, bool create_table_heap = true)
      -> TableInfo * {
    if (table_names_.count(table_name) != 0) {
      return NULL_TABLE_INFO;
    }

    // 构造表堆
    std::unique_ptr<TableHeap> table = nullptr;

    // 当 create_table_heap == false 时，意味着我们正在运行绑定器测试（此时不会提供事务），或者我们在没有缓冲池的 shell 中运行。
    // 在这种情况下不需要创建 TableHeap。
    if (create_table_heap) {
      table = std::make_unique<TableHeap>(bpm_);
    } else {
      // 否则，仅为绑定器测试创建一个空堆
      table = TableHeap::CreateEmptyHeap(create_table_heap);
    }

    // 获取新表的 OID
    const auto table_oid = next_table_oid_.fetch_add(1);

    // 构造表信息
    auto meta = std::make_unique<TableInfo>(schema, table_name, std::move(table), table_oid);
    auto *tmp = meta.get();

    // 更新内部追踪机制
    tables_.emplace(table_oid, std::move(meta));
    table_names_.emplace(table_name, table_oid);
    index_names_.emplace(table_name, std::unordered_map<std::string, index_oid_t>{});

    return tmp;
  }

  /**
   * 通过表名查询表的元数据。
   * @param table_name 表名
   * @return 表的元数据（非拥有者指针）
   */
  auto GetTable(const std::string &table_name) const -> TableInfo * {
    auto table_oid = table_names_.find(table_name);
    if (table_oid == table_names_.end()) {
      // 表未找到
      return NULL_TABLE_INFO;
    }

    auto meta = tables_.find(table_oid->second);
    BUSTUB_ASSERT(meta != tables_.end(), "Broken Invariant");

    return (meta->second).get();
  }

  /**
   * 通过 OID 查询表的元数据
   * @param table_oid 要查询的表的 OID
   * @return 表的元数据（非拥有者指针）
   */
  auto GetTable(table_oid_t table_oid) const -> TableInfo * {
    auto meta = tables_.find(table_oid);
    if (meta == tables_.end()) {
      return NULL_TABLE_INFO;
    }

    return (meta->second).get();
  }

  /**
   * 创建一个新的索引，填充表的现有数据并返回其元数据。
   * @param txn 创建索引的事务
   * @param index_name 新索引的名称
   * @param table_name 表名
   * @param schema 表的模式
   * @param key_schema 键的模式
   * @param key_attrs 键属性
   * @param keysize 键的大小
   * @param hash_function 索引的哈希函数
   * @return 新表的元数据（非拥有者指针）
   */
  template <class KeyType, class ValueType, class KeyComparator>
  auto CreateIndex(Transaction *txn, const std::string &index_name, const std::string &table_name, const Schema &schema,
                   const Schema &key_schema, const std::vector<uint32_t> &key_attrs, std::size_t keysize,
                   HashFunction<KeyType> hash_function, bool is_primary_key = false,
                   IndexType index_type = IndexType::HashTableIndex) -> IndexInfo * {
    // 拒绝创建不存在的表的请求
    if (table_names_.find(table_name) == table_names_.end()) {
      return NULL_INDEX_INFO;
    }

    // 如果表存在，表应当已在 index_names_ 中有条目
    BUSTUB_ASSERT((index_names_.find(table_name) != index_names_.end()), "Broken Invariant");

    // 确定请求的索引是否已经存在于该表中
    auto &table_indexes = index_names_.find(table_name)->second;
    if (table_indexes.find(index_name) != table_indexes.end()) {
      // 请求的索引已存在于该表中
      return NULL_INDEX_INFO;
    }

    // 构造索引元数据
    auto meta = std::make_unique<IndexMetadata>(index_name, table_name, &schema, key_attrs, is_primary_key);

    // 构造索引，接管元数据
    // TODO(Kyle): 我们应该更新 CreateIndex 的 API，
    // 使其能够指定索引类型本身，而不仅仅是键、值和比较器类型

    // TODO(chi): 支持哈希索引和 B 树索引
    std::unique_ptr<Index> index;
    if (index_type == IndexType::HashTableIndex) {
      std::cout<<"创建的索引类型是: HashTableIndex"<<std::endl;
      index = std::make_unique<ExtendibleHashTableIndex<KeyType, ValueType, KeyComparator>>(std::move(meta), bpm_,
                                                                                            hash_function);
    } else {
      BUSTUB_ASSERT(index_type == IndexType::BPlusTreeIndex, "Unsupported Index Type");
      std::cout<<"创建的索引类型是: BPlusTreeIndex"<<std::endl;
      index = std::make_unique<BPlusTreeIndex<KeyType, ValueType, KeyComparator>>(std::move(meta), bpm_);
    }

    // 用表堆中的所有元组填充索引
    auto *table_meta = GetTable(table_name);
    for (auto iter = table_meta->table_->MakeIterator(); !iter.IsEnd(); ++iter) {
      auto [meta, tuple] = iter.GetTuple();
      // 我们必须在这里默默地忽略错误，原因有很多...
      index->InsertEntry(tuple.KeyFromTuple(schema, key_schema, key_attrs), tuple.GetRid(), txn);
    }

    // 获取新索引的下一个 OID
    const auto index_oid = next_index_oid_.fetch_add(1);

    // 构造索引信息；IndexInfo 接管索引本身
    auto index_info = std::make_unique<IndexInfo>(key_schema, index_name, std::move(index), index_oid, table_name,
                                                  keysize, is_primary_key);
    auto *tmp = index_info.get();

    // 更新内部追踪
    indexes_.emplace(index_oid, std::move(index_info));
    table_indexes.emplace(index_name, index_oid);

    return tmp;
  }

  /**
   * 获取表 `table_name` 的索引 `index_name`。
   * @param index_name 要查询的索引名称
   * @param table_name 要查询的表名
   * @return 索引的元数据（非拥有者指针）
   */
  auto GetIndex(const std::string &index_name, const std::string &table_name) -> IndexInfo * {
    auto table = index_names_.find(table_name);
    if (table == index_names_.end()) {
      BUSTUB_ASSERT((table_names_.find(table_name) == table_names_.end()), "Broken Invariant");
      return NULL_INDEX_INFO;
    }

    auto &table_indexes = table->second;

    auto index_meta = table_indexes.find(index_name);
    if (index_meta == table_indexes.end()) {
      return NULL_INDEX_INFO;
    }

    auto index = indexes_.find(index_meta->second);
    BUSTUB_ASSERT((index != indexes_.end()), "Broken Invariant");

    return index->second.get();
  }

  /**
   * 获取表由 `table_oid` 标识的索引 `index_name`。
   * @param index_name 要查询的索引名称
   * @param table_oid 要查询的表的 OID
   * @return 索引的元数据（非拥有者指针）
   */
  auto GetIndex(const std::string &index_name, const table_oid_t table_oid) -> IndexInfo * {
    // 定位指定表 OID 的表元数据
    auto table_meta = tables_.find(table_oid);
    if (table_meta == tables_.end()) {
      // 表未找到
      return NULL_INDEX_INFO;
    }

    return GetIndex(index_name, table_meta->second->name_);
  }

  /**
   * 通过索引 OID 获取索引标识符。
   * @param index_oid 要查询的索引 OID
   * @return 索引的元数据（非拥有者指针）
   */
  auto GetIndex(index_oid_t index_oid) -> IndexInfo * {
    auto index = indexes_.find(index_oid);
    if (index == indexes_.end()) {
      return NULL_INDEX_INFO;
    }

    return index->second.get();
  }

  /**
   * 获取表 `table_name` 的所有索引。
   * @param table_name 要查询索引的表名
   * @return 索引的元数据向量，如果该表存在但没有创建索引，则返回空向量
   */
  auto GetTableIndexes(const std::string &table_name) const -> std::vector<IndexInfo *> {
    // 确保表存在
    if (table_names_.find(table_name) == table_names_.end()) {
      return std::vector<IndexInfo *>{};
    }

    auto table_indexes = index_names_.find(table_name);
    BUSTUB_ASSERT((table_indexes != index_names_.end()), "Broken Invariant");

    std::vector<IndexInfo *> indexes{};
    indexes.reserve(table_indexes->second.size());
    for (const auto &index_meta : table_indexes->second) {
      auto index = indexes_.find(index_meta.second);
      BUSTUB_ASSERT((index != indexes_.end()), "Broken Invariant");
      indexes.push_back(index->second.get());
    }

    return indexes;
  }

  auto GetTableNames() -> std::vector<std::string> {
    std::vector<std::string> result;
    for (const auto &x : table_names_) {
      result.push_back(x.first);
    }
    return result;
  }

 private:
  [[maybe_unused]] BufferPoolManager *bpm_;
  [[maybe_unused]] LockManager *lock_manager_;
  [[maybe_unused]] LogManager *log_manager_;

  /**
   * 表标识符 -> 表元数据的映射。
   *
   * 注意：`tables_` 拥有所有表元数据。
   */
  std::unordered_map<table_oid_t, std::unique_ptr<TableInfo>> tables_;

  /** 表名 -> 表标识符的映射。 */
  std::unordered_map<std::string, table_oid_t> table_names_;
  
  /** 下一个要使用的表标识符。 */
  std::atomic<table_oid_t> next_table_oid_{0};

  /**
   * 索引标识符 -> 索引元数据的映射。
   *
   * 注意：`indexes_` 拥有所有索引元数据。
   */
  std::unordered_map<index_oid_t, std::unique_ptr<IndexInfo>> indexes_;

  /** 表名 -> 索引名 -> 索引标识符的映射。 */
  std::unordered_map<std::string, std::unordered_map<std::string, index_oid_t>> index_names_;

  /** 下一个要使用的索引标识符。 */
  std::atomic<index_oid_t> next_index_oid_{0};
};

}  // namespace bustub
