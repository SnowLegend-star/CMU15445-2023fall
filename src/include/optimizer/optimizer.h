#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {

/**
 * 优化器接收一个 `AbstractPlanNode` 并输出一个优化后的 `AbstractPlanNode`。
 */
class Optimizer {
 public:
  explicit Optimizer(const Catalog &catalog, bool force_starter_rule)
      : catalog_(catalog), force_starter_rule_(force_starter_rule) {}

  auto Optimize(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  auto OptimizeCustom(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  void PredParser(const AbstractExpressionRef &predicate, std::vector<AbstractExpressionRef> *left_pred,
                  std::vector<AbstractExpressionRef> *right_pred);

 private:
  /**
   * @brief 合并执行相同投影操作的投影节点。
   * 相同的投影操作可能会在存在 `SELECT
   * *`、聚合操作，或需要重命名列时在规划器中产生。我们将合并这些投影操作，以提高执行效率。
   */
  auto OptimizeMergeProjection(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 将过滤条件合并到嵌套循环连接中。
   * 在规划器中，我们通常将笛卡尔积（cross join）和过滤条件计划成两个节点（分别为 `cross product` 和 `filter plan
   * node`）。我们可以将过滤条件合并到嵌套循环连接中，以提高效率。
   */
  auto OptimizeMergeFilterNLJ(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 将嵌套循环连接优化为哈希连接。
   * 在启动代码中，我们会检查仅包含一个相等条件的嵌套循环连接（NLJ）。你可以进一步支持优化具有多个相等条件的连接。
   */
  auto OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 将嵌套循环连接优化为索引连接。
   */
  auto OptimizeNLJAsIndexJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 消除永远为 `true` 的过滤条件。
   */
  auto OptimizeEliminateTrueFilter(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 将过滤条件合并到 `SeqScan` 的过滤谓词中。
   */
  auto OptimizeMergeFilterScan(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 重写用于嵌套循环连接的表达式。例如，如果我们有 `SELECT * FROM a, b WHERE a.x = b.y`，我们将
   * `#0.x = #0.y` 放入过滤计划节点。我们需要确定 `0.x` 和 `0.y` 应该分别属于 NLJ 中的哪个表（左表还是右表？），
   * 然后将其重写为 `#0.x = #1.y`。
   *
   * @param expr 过滤表达式
   * @param left_column_cnt 左表的列数
   * @param right_column_cnt 右表的列数
   */
  auto RewriteExpressionForJoin(const AbstractExpressionRef &expr, size_t left_column_cnt, size_t right_column_cnt)
      -> AbstractExpressionRef;

  /** @brief 检查谓词是否为 `true::boolean` 类型 */
  auto IsPredicateTrue(const AbstractExpressionRef &expr) -> bool;

  /**
   * @brief 如果表上有索引，则将 `ORDER BY` 优化为索引扫描。
   */
  auto OptimizeOrderByAsIndexScan(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 如果表上有索引，则将 `SeqScan` 优化为索引扫描。
   * @note 2023年秋季版本：仅支持哈希索引，并且仅支持点查找。
   */
  auto OptimizeSeqScanAsIndexScan(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /** @brief 检查索引是否可以匹配 */
  auto MatchIndex(const std::string &table_name, uint32_t index_key_idx)
      -> std::optional<std::tuple<index_oid_t, std::string>>;

  /**
   * @brief 将 `SORT + LIMIT` 优化为 `TopN`。
   */
  auto OptimizeSortLimitAsTopN(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef;

  /**
   * @brief 根据表名估算表的基数。这个方法在连接重排序时非常有用。由于 BusTub 当前不支持统计信息，
   * 这是获取表大小的唯一方法 :(
   *
   * @param table_name 表名
   * @return std::optional<size_t> 返回表的估算基数（行数）
   */
  auto EstimatedCardinality(const std::string &table_name) -> std::optional<size_t>;

  /**
   * Catalog 会在规划过程中使用。用户应该确保它的生命周期长于优化器，否则会造成悬挂引用。
   */
  const Catalog &catalog_;

  const bool force_starter_rule_;
};

}  // namespace bustub
