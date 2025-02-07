//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// constant_value_expression.h
//
// Identification: src/include/expression/constant_value_expression.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "execution/expressions/abstract_expression.h"

namespace bustub {
/**
 * ConstantValueExpression 代表常量.
 */
class ConstantValueExpression : public AbstractExpression {
 public:
  /** Creates a new constant value expression wrapping the given value. */
  explicit ConstantValueExpression(const Value &val) : AbstractExpression({}, val.GetTypeId()), val_(val) {}
  
  /**忽略 tuple 和 schema，直接返回 val_。
  因为常量不依赖于数据表，它的值是固定的**/
  auto Evaluate(const Tuple *tuple, const Schema &schema) const -> Value override { return val_; }

  auto EvaluateJoin(const Tuple *left_tuple, const Schema &left_schema, const Tuple *right_tuple,
                    const Schema &right_schema) const -> Value override {
    return val_;
  }

  /** @return the string representation of the plan node and its children */
  auto ToString() const -> std::string override { return val_.ToString(); }

  BUSTUB_EXPR_CLONE_WITH_CHILDREN(ConstantValueExpression);

  Value val_;
};
}  // namespace bustub
