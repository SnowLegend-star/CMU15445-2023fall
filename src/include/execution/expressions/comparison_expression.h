//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// comparison_expression.h
//
// Identification: src/include/expression/comparison_expression.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "catalog/schema.h"
#include "execution/expressions/abstract_expression.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"
#include "type/value_factory.h"

namespace bustub {

/** ComparisonType 定义了所有支持的比较运算符 */
enum class ComparisonType { Equal, NotEqual, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual };

// SELECT * FROM table1 t1 JOIN table2 t2 ON t1.id = t2.id
// 其中 t1.id = t2.id 就是 ComparisonExpression。

/**
 * ComparisonExpression代表两个被比较的表达式.
 */
class ComparisonExpression : public AbstractExpression {
 public:
  /** Creates a new comparison expression representing (left comp_type right). */
  ComparisonExpression(AbstractExpressionRef left, AbstractExpressionRef right, ComparisonType comp_type)
      : AbstractExpression({std::move(left), std::move(right)}, TypeId::BOOLEAN), comp_type_{comp_type} {}

  auto Evaluate(const Tuple *tuple, const Schema &schema) const -> Value override {
    Value lhs = GetChildAt(0)->Evaluate(tuple, schema);
    Value rhs = GetChildAt(1)->Evaluate(tuple, schema);
    return ValueFactory::GetBooleanValue(PerformComparison(lhs, rhs));
  }

  auto EvaluateJoin(const Tuple *left_tuple, const Schema &left_schema, const Tuple *right_tuple,
                    const Schema &right_schema) const -> Value override {
    Value lhs = GetChildAt(0)->EvaluateJoin(left_tuple, left_schema, right_tuple, right_schema);
    Value rhs = GetChildAt(1)->EvaluateJoin(left_tuple, left_schema, right_tuple, right_schema);
    return ValueFactory::GetBooleanValue(PerformComparison(lhs, rhs));
  }

  /** @return the string representation of the expression node and its children */
  auto ToString() const -> std::string override {
    return fmt::format("({}{}{})", *GetChildAt(0), comp_type_, *GetChildAt(1));
  }

  BUSTUB_EXPR_CLONE_WITH_CHILDREN(ComparisonExpression);

  ComparisonType comp_type_;

 private:
  auto PerformComparison(const Value &lhs, const Value &rhs) const -> CmpBool {
    switch (comp_type_) {
      case ComparisonType::Equal:
        return lhs.CompareEquals(rhs);
      case ComparisonType::NotEqual:
        return lhs.CompareNotEquals(rhs);
      case ComparisonType::LessThan:
        return lhs.CompareLessThan(rhs);
      case ComparisonType::LessThanOrEqual:
        return lhs.CompareLessThanEquals(rhs);
      case ComparisonType::GreaterThan:
        return lhs.CompareGreaterThan(rhs);
      case ComparisonType::GreaterThanOrEqual:
        return lhs.CompareGreaterThanEquals(rhs);
      default:
        BUSTUB_ASSERT(false, "Unsupported comparison type.");
    }
  }
};
}  // namespace bustub

template <>
struct fmt::formatter<bustub::ComparisonType> : formatter<string_view> {
  template <typename FormatContext>
  auto format(bustub::ComparisonType c, FormatContext &ctx) const {
    string_view name;
    switch (c) {
      case bustub::ComparisonType::Equal:
        name = "=";
        break;
      case bustub::ComparisonType::NotEqual:
        name = "!=";
        break;
      case bustub::ComparisonType::LessThan:
        name = "<";
        break;
      case bustub::ComparisonType::LessThanOrEqual:
        name = "<=";
        break;
      case bustub::ComparisonType::GreaterThan:
        name = ">";
        break;
      case bustub::ComparisonType::GreaterThanOrEqual:
        name = ">=";
        break;
      default:
        name = "Unknown";
        break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};
