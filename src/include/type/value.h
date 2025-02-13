//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// value.h
//
// Identification: src/include/type/value.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include "fmt/format.h"

#include "type/limits.h"
#include "type/type.h"

namespace bustub {

inline auto GetCmpBool(bool boolean) -> CmpBool { return boolean ? CmpBool::CmpTrue : CmpBool::CmpFalse; }

// Value 类是一个抽象类，表示存储在某种具体物理状态中的 SQL 数据的视图。
// 所有值都有类型和比较函数，但子类实现其他类型特定的功能。
class Value {
  // 好友类型类
  friend class Type;
  friend class NumericType;
  friend class IntegerParentType;
  friend class TinyintType;
  friend class SmallintType;
  friend class IntegerType;
  friend class BigintType;
  friend class DecimalType;
  friend class TimestampType;
  friend class BooleanType;
  friend class VarlenType;

 public:
  // 构造函数，使用 TypeId 类型来初始化值
  explicit Value(const TypeId type) : manage_data_(false), type_id_(type) { size_.len_ = BUSTUB_VALUE_NULL; }

  // 布尔值和小整数（TINYINT）
  Value(TypeId type, int8_t i);

  // 十进制（DECIMAL）
  Value(TypeId type, double d);
  Value(TypeId type, float f);

  // 小整数（SMALLINT）
  Value(TypeId type, int16_t i);

  // 整数（INTEGER）
  Value(TypeId type, int32_t i);

  // 大整数（BIGINT）
  Value(TypeId type, int64_t i);

  // 时间戳（TIMESTAMP）
  Value(TypeId type, uint64_t i);

  // 可变长度字符串（VARCHAR）
  Value(TypeId type, const char *data, uint32_t len, bool manage_data);
  Value(TypeId type, const std::string &data);

  // 默认构造函数，初始化为无效类型
  Value() : Value(TypeId::INVALID) {}

  // 拷贝构造函数
  Value(const Value &other);

  // 拷贝赋值运算符
  auto operator=(Value other) -> Value &;

  // 析构函数
  ~Value();

  // 交换两个值
  // NOLINTNEXTLINE
  friend void Swap(Value &first, Value &second) {
    std::swap(first.value_, second.value_);
    std::swap(first.size_, second.size_);
    std::swap(first.manage_data_, second.manage_data_);
    std::swap(first.type_id_, second.type_id_);
  }

  // 检查值是否为整数
  auto CheckInteger() const -> bool;

  // 检查该值是否可以与另一个值比较
  auto CheckComparable(const Value &o) const -> bool;

  // 获取值的类型
  inline auto GetTypeId() const -> TypeId { return type_id_; }

  // 获取可变长度数据的长度
  inline auto GetLength() const -> uint32_t { return Type::GetInstance(type_id_)->GetLength(*this); }

  // 访问原始的可变长度数据
  inline auto GetData() const -> const char * { return Type::GetInstance(type_id_)->GetData(*this); }

  // 类型转换：将值转换为指定的类型
  template <class T>
  inline auto GetAs() const -> T {
    return *reinterpret_cast<const T *>(&value_);
  }

  // 将值转换为指定的类型
  inline auto CastAs(const TypeId type_id) const -> Value {
    return Type::GetInstance(type_id_)->CastAs(*this, type_id);
  }

  // 如果两个值相等，返回 true
  inline auto CompareExactlyEquals(const Value &o) const -> bool {
    if (this->IsNull() && o.IsNull()) {
      return true;
    }
    return (Type::GetInstance(type_id_)->CompareEquals(*this, o)) == CmpBool::CmpTrue;
  }

  // 比较方法
  inline auto CompareEquals(const Value &o) const -> CmpBool {
    return Type::GetInstance(type_id_)->CompareEquals(*this, o);
  }
  inline auto CompareNotEquals(const Value &o) const -> CmpBool {
    return Type::GetInstance(type_id_)->CompareNotEquals(*this, o);
  }
  inline auto CompareLessThan(const Value &o) const -> CmpBool {
    return Type::GetInstance(type_id_)->CompareLessThan(*this, o);
  }
  inline auto CompareLessThanEquals(const Value &o) const -> CmpBool {
    return Type::GetInstance(type_id_)->CompareLessThanEquals(*this, o);
  }
  inline auto CompareGreaterThan(const Value &o) const -> CmpBool {
    return Type::GetInstance(type_id_)->CompareGreaterThan(*this, o);
  }
  inline auto CompareGreaterThanEquals(const Value &o) const -> CmpBool {
    return Type::GetInstance(type_id_)->CompareGreaterThanEquals(*this, o);
  }

  // 其他数学函数
  inline auto Add(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Add(*this, o); }
  inline auto Subtract(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Subtract(*this, o); }
  inline auto Multiply(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Multiply(*this, o); }
  inline auto Divide(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Divide(*this, o); }
  inline auto Modulo(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Modulo(*this, o); }
  inline auto Min(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Min(*this, o); }
  inline auto Max(const Value &o) const -> Value { return Type::GetInstance(type_id_)->Max(*this, o); }
  inline auto Sqrt() const -> Value { return Type::GetInstance(type_id_)->Sqrt(*this); }

  // 操作空值
  inline auto OperateNull(const Value &o) const -> Value { return Type::GetInstance(type_id_)->OperateNull(*this, o); }

  // 检查是否为零
  inline auto IsZero() const -> bool { return Type::GetInstance(type_id_)->IsZero(*this); }

  // 检查是否为空值
  inline auto IsNull() const -> bool { return size_.len_ == BUSTUB_VALUE_NULL; }

  // 序列化该值到给定的存储空间。inlined 参数指示是否允许将该值内联到存储空间中，
  // 或者仅存储该值的引用。如果 inlined 为 false，我们可能会使用提供的数据池为该值分配空间，
  // 并将引用存储到分配的池空间中。
  inline void SerializeTo(char *storage) const { Type::GetInstance(type_id_)->SerializeTo(*this, storage); }

  // 从给定的存储空间反序列化一个值
  inline static auto DeserializeFrom(const char *storage, const TypeId type_id) -> Value {
    return Type::GetInstance(type_id)->DeserializeFrom(storage);
  }

  // 返回该值的字符串表示
  inline auto ToString() const -> std::string { return Type::GetInstance(type_id_)->ToString(*this); }

  // 创建该值的副本
  inline auto Copy() const -> Value { return Type::GetInstance(type_id_)->Copy(*this); }

 protected:
  // 实际的值项
  union Val {
    int8_t boolean_;            // 布尔值
    int8_t tinyint_;            // 小整数
    int16_t smallint_;          // 小整数
    int32_t integer_;           // 整数
    int64_t bigint_;            // 大整数
    double decimal_;            // 十进制数
    uint64_t timestamp_;        // 时间戳
    char *varlen_;              // 可变长度字符串
    const char *const_varlen_;  // 常量可变长度字符串
  } value_;

  union {
    uint32_t len_;         // 长度
    TypeId elem_type_id_;  // 元素类型
  } size_;

  bool manage_data_;  // 是否管理数据
  TypeId type_id_;    // 数据类型
};
}  // namespace bustub

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<bustub::Value, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const bustub::Value &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x.ToString(), ctx);
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>, std::enable_if_t<std::is_base_of<bustub::Value, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::unique_ptr<bustub::Value> &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x->ToString(), ctx);
  }
};
