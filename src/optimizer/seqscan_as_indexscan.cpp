#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/index_scan_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "optimizer/optimizer.h"
#include "type/type.h"

namespace bustub {

auto Optimizer::OptimizeSeqScanAsIndexScan(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement seq scan with predicate -> index scan optimizer rule
  // The Filter Predicate Pushdown has been enabled for you in optimizer.cpp when forcing starter rule
  std::vector<AbstractPlanNodeRef> children;
  for(const auto&child:plan->GetChildren()){
    children.emplace_back(OptimizeSeqScanAsIndexScan(child));
  }

  auto optimized_plan=plan->CloneWithChildren(std::move(children));
  // 尝试将seqScan转化为indexScan
  if(optimized_plan->GetType()==PlanType::SeqScan){
    // 将当前plan转化为seq_plan
    const auto &seq_plan=dynamic_cast<const SeqScanPlanNode &>(*optimized_plan);
    //如果seq没有filter_predicate, 那就是将整个table传上去, 也没必要用到索引
    if(seq_plan.filter_predicate_!=nullptr){
      auto table_info=catalog_.GetTable(seq_plan.table_oid_);
      auto table_indexes=catalog_.GetTableIndexes(seq_plan.table_name_);
      // 如果filter_pre是一个逻辑谓词 例如v1 = 1 AND v2 = 2, 则直接返回, 因为bustub的索引都是单列的, 两者不可能匹配
      if(auto tmp=dynamic_cast<LogicExpression*>(seq_plan.filter_predicate_.get());
        tmp!=nullptr&&table_indexes.empty()){
          return optimized_plan;
      }
      // 这个filter_pri实际上是一个compExpr 例如where v1=1
      auto filter_comp_expr=dynamic_cast<ComparisonExpression*>(seq_plan.filter_predicate_.get());
      if(filter_comp_expr!=nullptr&&filter_comp_expr->comp_type_==ComparisonType::Equal){
        // 提取filter_pre中的列名
        auto filter_colval_expr=dynamic_cast<ColumnValueExpression*>(filter_comp_expr->children_[0].get());

        if(filter_colval_expr==nullptr){  // 其实到这一步基本不可能出问题了
          return optimized_plan;
        }
        std::vector<uint32_t> filter_colval_expr_ids{filter_colval_expr->GetColIdx()};  // 装模作样把一个元素塞进vector里
        // 开始判断filter_pre和索引的关系
        for(const auto &index:table_indexes){
          auto index_columns=index->index_->GetKeyAttrs();
          if(index_columns==filter_colval_expr_ids){
            auto pred_key=dynamic_cast<ConstantValueExpression*>(filter_comp_expr->children_[1].get());
            return std::make_shared<IndexScanPlanNode>(optimized_plan->output_schema_,table_info->oid_,index->index_oid_,seq_plan.filter_predicate_,pred_key);
          }
        }
      }
    }
  }
  return optimized_plan;
}

}  // namespace bustub
