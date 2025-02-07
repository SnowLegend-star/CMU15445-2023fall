//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// column.h
//
// Identification: src/include/catalog/column.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "fmt/format.h"

#include "common/exception.h"
#include "common/macros.h"
#include "type/type.h"

namespace bustub {
class AbstractExpression;

class Column {
  friend class Schema;

 public:
  /**
   * 非可变长度构造函数，用于创建列。
   * @param column_name 列的名称
   * @param type 列的数据类型
   */
  Column(std::string column_name, TypeId type)
      : column_name_(std::move(column_name)), column_type_(type), fixed_length_(TypeSize(type)) {
    BUSTUB_ASSERT(type != TypeId::VARCHAR, "Wrong constructor for VARCHAR type.");
  }

  /**
   * 可变长度构造函数，用于创建列。
   * @param column_name 列的名称
   * @param type 列的数据类型
   * @param length 可变长度的长度
   * @param expr 创建该列使用的表达式
   */
  Column(std::string column_name, TypeId type, uint32_t length)
      : column_name_(std::move(column_name)),
        column_type_(type),
        fixed_length_(TypeSize(type)),
        variable_length_(length) {
    BUSTUB_ASSERT(type == TypeId::VARCHAR, "Wrong constructor for non-VARCHAR type.");
  }

  /**
   * 复制一个列并更改名称。
   * @param column_name 新的列名
   * @param column 原始列
   */
  Column(std::string column_name, const Column &column)
      : column_name_(std::move(column_name)),
        column_type_(column.column_type_),
        fixed_length_(column.fixed_length_),
        variable_length_(column.variable_length_),
        column_offset_(column.column_offset_) {}

  /** @return 列的名称 */
  auto GetName() const -> std::string { return column_name_; }

  /** @return 列的长度 */
  auto GetLength() const -> uint32_t {
    if (IsInlined()) {
      return fixed_length_;
    }
    return variable_length_;
  }

  /** @return 列的固定长度 */
  auto GetFixedLength() const -> uint32_t { return fixed_length_; }

  /** @return 列的可变长度 */
  auto GetVariableLength() const -> uint32_t { return variable_length_; }

  /** @return 列在元组中的偏移量 */
  auto GetOffset() const -> uint32_t { return column_offset_; }

  /** @return 列的数据类型 */
  auto GetType() const -> TypeId { return column_type_; }

  /** @return 如果列是内联的，返回 true，否则返回 false */
  auto IsInlined() const -> bool { return column_type_ != TypeId::VARCHAR; }

  /** @return 列的字符串表示 */
  auto ToString(bool simplified = true) const -> std::string;

 private:
  /**
   * 返回类型的字节大小。
   * @param type 需要确定大小的类型
   * @return 类型的字节大小
   */
  static auto TypeSize(TypeId type) -> uint8_t {
    switch (type) {
      case TypeId::BOOLEAN:
      case TypeId::TINYINT:
        return 1;
      case TypeId::SMALLINT:
        return 2;
      case TypeId::INTEGER:
        return 4;
      case TypeId::BIGINT:
      case TypeId::DECIMAL:
      case TypeId::TIMESTAMP:
        return 8;
      case TypeId::VARCHAR:
        // TODO(Amadou): 确认这个大小。
        return 12;
      default: {
        UNREACHABLE("Cannot get size of invalid type. ");
      }
    }
  }

  /** 列的名称。 */
  std::string column_name_;

  /** 列值的类型。 */
  TypeId column_type_;

  /** 对于非内联列，这是指针的大小。否则，为固定长度列的大小。 */
  uint32_t fixed_length_;

  /** 对于内联列，值为 0。否则，为可变长度列的长度。 */
  uint32_t variable_length_{0};

  /** 列在元组中的偏移量。 */
  uint32_t column_offset_{0};
};


}  // namespace bustub

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<bustub::Column, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const bustub::Column &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x.ToString(), ctx);
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>, std::enable_if_t<std::is_base_of<bustub::Column, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::unique_ptr<bustub::Column> &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x->ToString(), ctx);
  }
};
