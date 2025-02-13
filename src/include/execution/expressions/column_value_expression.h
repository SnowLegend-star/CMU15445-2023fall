//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// column_value_expression.h
//
// Identification: src/include/expression/column_value_expression.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "catalog/schema.h"
#include "execution/expressions/abstract_expression.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * ColumnValueExpression 维护了一个元组索引和相对于特定模式或连接的列索引
 */
class ColumnValueExpression : public AbstractExpression {
 public:
  /**
   * ColumnValueExpression 是对 "Table.member" 的抽象表示，基于索引。
   * ⚡ 注意：这个类是叶子节点（Leaf Expression），所以它的 children_ 为空 {}
   * @param tuple_idx {元组索引 0 = 连接的左侧，元组索引 1 = 连接的右侧}
   * @param col_idx 列在模式中的索引
   * @param ret_type 表达式的返回类型
   */
  ColumnValueExpression(uint32_t tuple_idx, uint32_t col_idx, TypeId ret_type)
      : AbstractExpression({}, ret_type), tuple_idx_{tuple_idx}, col_idx_{col_idx} {}

  auto Evaluate(const Tuple *tuple, const Schema &schema) const -> Value override {
    return tuple->GetValue(&schema, col_idx_);
  }

  auto EvaluateJoin(const Tuple *left_tuple, const Schema &left_schema, const Tuple *right_tuple,
                    const Schema &right_schema) const -> Value override {
    return tuple_idx_ == 0 ? left_tuple->GetValue(&left_schema, col_idx_)
                           : right_tuple->GetValue(&right_schema, col_idx_);
  }

  auto GetTupleIdx() const -> uint32_t { return tuple_idx_; }
  auto GetColIdx() const -> uint32_t { return col_idx_; }

  /** @return the string representation of the plan node and its children */
  // "#1.3" 代表 右表（1）中的第 3 列。
  // 终于找到这句话的跟脚了
  auto ToString() const -> std::string override { return fmt::format("#{}.{}", tuple_idx_, col_idx_); }

  BUSTUB_EXPR_CLONE_WITH_CHILDREN(ColumnValueExpression);

 private:
  /** 元组索引 0 = 连接的左侧，元组索引 1 = 连接的右侧 */
  uint32_t tuple_idx_;
  /** Column index refers to the index within the schema of the tuple, e.g. schema {A,B,C} has indexes {0,1,2} */
  /** 列索引指的是元组模式中的索引，例如模式 {A,B,C} 对应的索引是 {0,1,2} */
  uint32_t col_idx_;  // 我觉得这里就和attrs是一回事
};
}  // namespace bustub
