//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index.h
//
// Identification: src/include/storage/index/index.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/schema.h"
#include "storage/table/tuple.h"
#include "type/value.h"

namespace bustub {

class Transaction;

/**
 * class IndexMetadata - 保存索引对象的元数据。
 *
 * 元数据对象维护索引的元组模式和键属性，因为外部调用者不知道实际的
 * 索引键结构，因此索引有责任维护这种映射关系，并在元组键和索引键之间进行转换。
 */
class IndexMetadata {
 public:
  IndexMetadata() = delete;

  /**
   * 构造一个新的 IndexMetadata 实例。
   * @param index_name 索引的名称
   * @param table_name 索引创建所在的表的名称
   * @param tuple_schema 被索引键的模式
   * @param key_attrs 索引列与基础表列的映射关系
   */
  IndexMetadata(std::string index_name, std::string table_name, const Schema *tuple_schema,
                std::vector<uint32_t> key_attrs, bool is_primary_key)
      : name_(std::move(index_name)),
        table_name_(std::move(table_name)),
        key_attrs_(std::move(key_attrs)),
        is_primary_key_(is_primary_key) {
    key_schema_ = std::make_shared<Schema>(Schema::CopySchema(tuple_schema, key_attrs_));
  }

  ~IndexMetadata() = default;

  /** @return 索引的名称 */
  inline auto GetName() const -> const std::string & { return name_; }

  /** @return 索引创建所在表的名称 */
  inline auto GetTableName() -> const std::string & { return table_name_; }

  /** @return 一个表示索引键的模式对象指针 */
  inline auto GetKeySchema() const -> Schema * { return key_schema_.get(); }

  /**
   * @return 索引键中的列数（不包括元组键中的列）
   *
   * 注意：此函数必须在 cpp 源文件中定义，因为它使用了 catalog::Schema 中的成员，
   * 该成员在这里不可见。
   */
  auto GetIndexColumnCount() const -> std::uint32_t { return static_cast<uint32_t>(key_attrs_.size()); }

  /** @return 索引列与基础表列的映射关系 */
  inline auto GetKeyAttrs() const -> const std::vector<uint32_t> & { return key_attrs_; }

  /** @return 是否是主键 */
  inline auto IsPrimaryKey() const -> bool { return is_primary_key_; }

  /** @return 用于调试的字符串表示 */
  auto ToString() const -> std::string {
    std::stringstream os;

    os << "IndexMetadata[" 
       << "Name = " << name_ << ", "
       << "Type = B+Tree, "
       << "Table name = " << table_name_ << "] :: ";
    os << key_schema_->ToString();

    return os.str();
  }

 private:
  /** 索引的名称 */
  std::string name_;
  /** 索引创建所在的表的名称 */
  std::string table_name_;
  /** 键模式与元组模式之间的映射关系 */
  const std::vector<uint32_t> key_attrs_;
  /** 索引键的模式 */
  std::shared_ptr<Schema> key_schema_;
  /** 是否为主键 */
  bool is_primary_key_;
};

/////////////////////////////////////////////////////////////////////
// 索引类定义
/////////////////////////////////////////////////////////////////////

/**
 * class Index - 各种类型的索引的基类
 *
 * 索引结构主要维护底层表的模式信息，以及索引键和元组键之间的映射关系，
 * 提供了一种抽象的方式供外部世界与底层索引实现交互，而不暴露
 * 实际实现的接口。
 *
 * 索引对象还处理谓词扫描，除了简单的插入、删除、谓词插入、点查询和完整索引扫描。
 * 谓词扫描仅支持合取式，并且可能根据谓词中表达式的类型进行优化。
 */
class Index {
 public:
  /**
   * 构造一个新的 Index 实例。
   * @param metadata 索引元数据的拥有指针
   */
  explicit Index(std::unique_ptr<IndexMetadata> &&metadata) : metadata_{std::move(metadata)} {}

  virtual ~Index() = default;

  /** @return 一个指向与索引关联的元数据对象的非拥有指针 */
  auto GetMetadata() const -> IndexMetadata * { return metadata_.get(); }

  /** @return 索引列的数量 */
  auto GetIndexColumnCount() const -> std::uint32_t { return metadata_->GetIndexColumnCount(); }

  /** @return 索引的名称 */
  auto GetName() const -> const std::string & { return metadata_->GetName(); }

  /** @return 索引键的模式 */
  auto GetKeySchema() const -> Schema * { return metadata_->GetKeySchema(); }

  /** @return 索引键的属性 */
  auto GetKeyAttrs() const -> const std::vector<uint32_t> & { return metadata_->GetKeyAttrs(); }

  /** @return 用于调试的字符串表示 */
  auto ToString() const -> std::string {
    std::stringstream os;
    os << "INDEX: (" << GetName() << ")";
    os << metadata_->ToString();
    return os.str();
  }

  ///////////////////////////////////////////////////////////////////
  // 点修改
  ///////////////////////////////////////////////////////////////////

  /**
   * 将一个条目插入到索引中。
   * @param key 索引键
   * @param rid 与该键关联的 RID
   * @param transaction 事务上下文
   * @returns 插入是否成功
   */
  virtual auto InsertEntry(const Tuple &key, RID rid, Transaction *transaction) -> bool = 0;

  /**
   * 通过键删除索引条目。
   * @param key 索引键
   * @param rid 与该键关联的 RID（未使用）
   * @param transaction 事务上下文
   */
  virtual void DeleteEntry(const Tuple &key, RID rid, Transaction *transaction) = 0;

  /**
   * 在索引中查找提供的键。
   * @param key 索引键
   * @param result 存放搜索结果的 RID 集合
   * @param transaction 事务上下文
   */
  virtual void ScanKey(const Tuple &key, std::vector<RID> *result, Transaction *transaction) = 0;

 private:
  /** 索引结构拥有其元数据 */
  std::unique_ptr<IndexMetadata> metadata_;
};

}  // namespace bustub
