//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// abstract_expression.h
//
// Identification: src/include/expression/abstract_expression.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/schema.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"

#define BUSTUB_EXPR_CLONE_WITH_CHILDREN(cname)                                                                   \
  auto CloneWithChildren(std::vector<AbstractExpressionRef> children) const->std::unique_ptr<AbstractExpression> \
      override {                                                                                                 \
    auto expr = cname(*this);                                                                                    \
    expr.children_ = children;                                                                                   \
    return std::make_unique<cname>(std::move(expr));                                                             \
  }

namespace bustub {

class AbstractExpression;
using AbstractExpressionRef = std::shared_ptr<AbstractExpression>;

/**
 * AbstractExpression 是系统中所有表达式的基类。
 * 表达式被建模为树形结构，即每个表达式可能有多个子节点。
 * 主要针对于where clause
 */
class AbstractExpression {
 public:
  /**
   * 使用给定的子节点和返回类型创建一个新的 AbstractExpression。
   * @param children 该抽象表达式的子节点
   * @param ret_type 该抽象表达式在求值时的返回类型
   */
  AbstractExpression(std::vector<AbstractExpressionRef> children, TypeId ret_type)
      : children_{std::move(children)}, ret_type_{ret_type} {}

  /** 虚析构函数。 */
  virtual ~AbstractExpression() = default;

  /** @return 使用给定的元组和模式求值后得到的值 */
  /** 返回value本质是bool类型 **/
  virtual auto Evaluate(const Tuple *tuple, const Schema &schema) const -> Value = 0;

  /**
   * 返回通过评估 JOIN 操作得到的值。 跨表比较  返回value本质是bool类型
   * @param left_tuple 左边的元组
   * @param left_schema 左边元组的模式
   * @param right_tuple 右边的元组
   * @param right_schema 右边元组的模式
   * @return 通过在左右元组上执行 JOIN 得到的值
   */
  virtual auto EvaluateJoin(const Tuple *left_tuple, const Schema &left_schema, const Tuple *right_tuple,
                            const Schema &right_schema) const -> Value = 0;

  /** @return 该表达式的第 child_idx 个子节点 */
  auto GetChildAt(uint32_t child_idx) const -> const AbstractExpressionRef & { return children_[child_idx]; }

  /** @return 该表达式的所有子节点，子节点的顺序可能很重要 */
  auto GetChildren() const -> const std::vector<AbstractExpressionRef> & { return children_; }

  /** @return 如果该表达式被求值时返回的类型 */
  virtual auto GetReturnType() const -> TypeId { return ret_type_; }

  /** @return 该计划节点及其子节点的字符串表示 */
  virtual auto ToString() const -> std::string { return "<unknown>"; }

  /** @return 一个具有新子节点的新的表达式 */
  virtual auto CloneWithChildren(std::vector<AbstractExpressionRef> children) const
      -> std::unique_ptr<AbstractExpression> = 0;

  /** 该表达式的子节点。注意，子节点的出现顺序可能很重要。 */
  std::vector<AbstractExpressionRef> children_;

 private:
  /** 该表达式的返回类型。 */
  TypeId ret_type_;
};

}  // namespace bustub

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<bustub::AbstractExpression, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const T &x, FormatCtx &ctx) const {
    return fmt::formatter<std::string>::format(x.ToString(), ctx);
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>, std::enable_if_t<std::is_base_of<bustub::AbstractExpression, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::unique_ptr<T> &x, FormatCtx &ctx) const {
    if (x != nullptr) {
      return fmt::formatter<std::string>::format(x->ToString(), ctx);
    }
    return fmt::formatter<std::string>::format("", ctx);
  }
};

template <typename T>
struct fmt::formatter<std::shared_ptr<T>, std::enable_if_t<std::is_base_of<bustub::AbstractExpression, T>::value, char>>
    : fmt::formatter<std::string> {
  template <typename FormatCtx>
  auto format(const std::shared_ptr<T> &x, FormatCtx &ctx) const {
    if (x != nullptr) {
      return fmt::formatter<std::string>::format(x->ToString(), ctx);
    }
    return fmt::formatter<std::string>::format("", ctx);
  }
};
