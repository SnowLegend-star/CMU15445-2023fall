//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_plan.h
//
// Identification: src/include/execution/plans/index_scan_plan.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <utility>

#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {
/**
 * IndexScanPlanNode 用于标识一个需要扫描的表，并且可以附加一个可选的谓词。
 */
class IndexScanPlanNode : public AbstractPlanNode {
 public:
  /**
   * 创建一个新的索引扫描计划节点，并附加过滤谓词。
   * @param output 该扫描计划节点的输出格式
   * @param table_oid 要扫描的表的标识符
   * @param filter_predicate 要下推到索引扫描的谓词
   * @param pred_key 用于点查找的键
   */
  IndexScanPlanNode(SchemaRef output, table_oid_t table_oid, index_oid_t index_oid,
                    AbstractExpressionRef filter_predicate = nullptr, ConstantValueExpression *pred_key = nullptr)
      : AbstractPlanNode(std::move(output), {}),
        table_oid_(table_oid),
        index_oid_(index_oid),
        filter_predicate_(std::move(filter_predicate)),
        pred_key_(pred_key) {}

  auto GetType() const -> PlanType override { return PlanType::IndexScan; }

  /** @return 应该扫描的表的标识符 */
  auto GetIndexOid() const -> index_oid_t { return index_oid_; }

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(IndexScanPlanNode);

  /** 索引是基于哪张表创建的。 */
  table_oid_t table_oid_;

  /** 需要扫描的索引。 */
  index_oid_t index_oid_;

  /** 用于在索引扫描中过滤的谓词。
   * 对于 2023 年秋季学期，在你实现了从顺序扫描到索引扫描的优化规则之后，
   * 我们可以使用这个谓词来执行索引点查找。
   */
  AbstractExpressionRef filter_predicate_;

  /**
   * 要查找的常量值键。
   * 例如，当处理 "WHERE v = 1" 时，我们可以把常量值 1 存储在这里。
   */
  const ConstantValueExpression *pred_key_;

  // 添加你需要的用于索引查找的其他内容

 protected:
  auto PlanNodeToString() const -> std::string override {
    if (filter_predicate_) {
      return fmt::format("IndexScan {{ index_oid={}, filter={} }}", index_oid_, filter_predicate_);
    }
    return fmt::format("IndexScan {{ index_oid={} }}", index_oid_);
  }
};

}  // namespace bustub
