//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_plan.h
//
// Identification: src/include/execution/plans/aggregation_plan.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "binder/bound_order_by.h"
#include "common/util/hash_util.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"
#include "type/value.h"

namespace bustub {

/**
 * WindowFunctionType 枚举列举了系统中所有可能的窗口函数
 */
enum class WindowFunctionType { CountStarAggregate, CountAggregate, SumAggregate, MinAggregate, MaxAggregate, Rank };

class WindowFunctionPlanNode : public AbstractPlanNode {
 public:
  /**
   * 构造一个新的 WindowFunctionPlanNode 实例。
   * @param output_schema 输出格式的计划节点
   * @param child 聚合数据的子计划
   * @param window_func_indexes 窗口函数的索引
   * @param columns 所有列，包括窗口函数的占位符
   * @param partition_bys 窗口函数的 PARTITION BY 子句
   * @param order_bys 窗口函数的 ORDER BY 子句
   * @param funcions 我们正在聚合的表达式
   * @param window_func_types 我们正在聚合的类型
   *
   * 窗口聚合与普通聚合不同，因为它为每个输入的行输出一行，
   * 并且可以与普通选择的列结合使用。WindowFunctionPlanNode 中的列包含了普通选择的列
   * 和窗口聚合的占位符列。
   *
   * 例如，如果我们有如下查询：
   *    SELECT 0.1, 0.2, SUM(0.3) OVER (PARTITION BY 0.2 ORDER BY 0.3), SUM(0.4) OVER (PARTITION BY 0.1 ORDER BY
   * 0.2,0.3) FROM table;
   *
   * WindowFunctionPlanNode 应包含以下结构：
   *    columns: std::vector<AbstractExpressionRef>{0.1, 0.2, 0.-1(占位符), 0.-1(占位符)}
   *    partition_bys: std::vector<std::vector<AbstractExpressionRef>>{{0.2}, {0.1}}
   *    order_bys: std::vector<std::vector<AbstractExpressionRef>>{{0.3}, {0.2,0.3}}

   *    functions: std::vector<AbstractExpressionRef>{0.3, 0.4}
   *    window_func_types: std::vector<WindowFunctionType>{SumAggregate, SumAggregate}

   * 这次所有的window_func都处理相同的order by
   * 这个傻逼functions到底是啥呢？难道特指一个列吗？
   * 那这个傻逼window_func_indexes又是什么√⑧？我去你的 要是没有建立索引就不用玩儿了是吧
   * damn! 这里window_func_indexes指的是window_func的个数, 而不是索引的意思。我才是傻逼
   */
  WindowFunctionPlanNode(SchemaRef output_schema, AbstractPlanNodeRef child, std::vector<uint32_t> window_func_indexes,
                         std::vector<AbstractExpressionRef> columns,
                         std::vector<std::vector<AbstractExpressionRef>> partition_bys,
                         std::vector<std::vector<std::pair<OrderByType, AbstractExpressionRef>>> order_bys,
                         std::vector<AbstractExpressionRef> functions,
                         std::vector<WindowFunctionType> window_func_types)
      : AbstractPlanNode(std::move(output_schema), {std::move(child)}), columns_(std::move(columns)) {
    for (uint32_t i = 0; i < window_func_indexes.size(); i++) {
      window_functions_[window_func_indexes[i]] =
          WindowFunction{functions[i], window_func_types[i], partition_bys[i], order_bys[i]};
    }
  }

  /** @return 计划节点的类型 */
  auto GetType() const -> PlanType override { return PlanType::Window; }

  /** @return 该聚合计划节点的子节点 */
  auto GetChildPlan() const -> AbstractPlanNodeRef {
    BUSTUB_ASSERT(GetChildren().size() == 1, "窗口聚合应该只有一个子节点。");
    return GetChildAt(0);
  }

  static auto InferWindowSchema(const std::vector<AbstractExpressionRef> &columns) -> Schema;

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(WindowFunctionPlanNode);

  struct WindowFunction {
    AbstractExpressionRef function_;
    WindowFunctionType type_;
    std::vector<AbstractExpressionRef> partition_by_;
    std::vector<std::pair<OrderByType, AbstractExpressionRef>> order_by_;
  };

  /** 所有列表达式 */
  std::vector<AbstractExpressionRef> columns_;

  std::unordered_map<uint32_t, WindowFunction> window_functions_;

 protected:
  auto PlanNodeToString() const -> std::string override;
};

/** WindowFunctionKey 表示聚合操作中的一个键 */
struct WindowFunctionKey {
  /** GROUP BY 值 */
  std::vector<Value> group_bys_;

  /**
   * 比较两个聚合键是否相等。
   * @param other 另一个聚合键进行比较
   * @return 如果两个聚合键的 GROUP BY 表达式相同，则返回 `true`，否则返回 `false`
   */
  auto operator==(const WindowFunctionKey &other) const -> bool {
    for (uint32_t i = 0; i < other.group_bys_.size(); i++) {
      if (group_bys_[i].CompareEquals(other.group_bys_[i]) != CmpBool::CmpTrue) {
        return false;
      }
    }
    return true;
  }
};

// /** WindowFunctionValue 表示每个运行中的聚合的值 */
// struct WindowFunctionValue {
//   /** 聚合值 */
//   Value val;
// };

}  // namespace bustub

namespace std {
///** 为 WindowFunctionKey 实现 std::hash */
template <>
struct hash<bustub::WindowFunctionKey> {
  auto operator()(const bustub::WindowFunctionKey &agg_key) const -> std::size_t {
    size_t curr_hash = 0;
    for (const auto &key : agg_key.group_bys_) {
      if (!key.IsNull()) {
        curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&key));
      }
    }
    return curr_hash;
  }
};
}  // namespace std

template <>
struct fmt::formatter<bustub::WindowFunctionPlanNode::WindowFunction> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const bustub::WindowFunctionPlanNode::WindowFunction &x, FormatContext &ctx) const {
    return formatter<std::string>::format(fmt::format("{{ function_arg={}, type={}, partition_by={}, order_by={} }}",
                                                      x.function_, x.type_, x.partition_by_, x.order_by_),
                                          ctx);
  }
};

template <>
struct fmt::formatter<bustub::WindowFunctionType> : formatter<std::string> {
  template <typename FormatContext>
  auto format(bustub::WindowFunctionType c, FormatContext &ctx) const {
    using bustub::WindowFunctionType;
    std::string name = "unknown";
    switch (c) {
      case WindowFunctionType::CountStarAggregate:
        name = "count_star";
        break;
      case WindowFunctionType::CountAggregate:
        name = "count";
        break;
      case WindowFunctionType::SumAggregate:
        name = "sum";
        break;
      case WindowFunctionType::MinAggregate:
        name = "min";
        break;
      case WindowFunctionType::MaxAggregate:
        name = "max";
        break;
      case WindowFunctionType::Rank:
        name = "rank";
        break;
    }
    return formatter<std::string>::format(name, ctx);
  }
};
