//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.h
//
// Identification: src/include/execution/executors/nested_loop_join_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "common/rid.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * NestedLoopJoinExecutor 执行两个表的嵌套循环连接（nested-loop JOIN）。
 */
 class NestedLoopJoinExecutor : public AbstractExecutor {
  public:
   /**
    * 构造一个新的 NestedLoopJoinExecutor 实例。
    * @param exec_ctx 执行上下文
    * @param plan 要执行的嵌套循环连接计划
    * @param left_executor 生成左侧连接元组的子执行器
    * @param right_executor 生成右侧连接元组的子执行器
    */
   NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                          std::unique_ptr<AbstractExecutor> &&left_executor,
                          std::unique_ptr<AbstractExecutor> &&right_executor);
 
   /** 初始化连接 */
   void Init() override;
 
   /**
    * 输出连接的下一个元组。
    * @param[out] tuple 连接生成的下一个元组
    * @param[out] rid 连接生成的下一个元组的 RID，嵌套循环连接中不使用此值
    * @return 如果生成了一个元组，则返回 true；如果没有更多元组，则返回 false。
    */
   auto Next(Tuple *tuple, RID *rid) -> bool override;
 
   /** @return 插入操作的输出模式 */
   auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };
 
  /** inner连接 **/
  auto InnerJoin()->bool;

  /** left连接 **/
  auto LeftJoin()->bool;

  private:
   /** 要执行的 NestedLoopJoin 计划节点。 */
   const NestedLoopJoinPlanNode *plan_;
   std::unique_ptr<AbstractExecutor> left_executor_;
   std::unique_ptr<AbstractExecutor> right_executor_;
  //  std::vector<RID>::iterator left_table_iter_; // 左侧表的迭代器 naive方法中, 表是无序的, 用iter也没用
  Tuple left_cur_tuple_;  // 记录left table正在处理的元组
  bool left_status_;  // 此次获取left table的元素是否成功
  bool tuple_match_;   // 这次left tuple是否找到了对应的right tuple  left tuple必然会找到对应的
 }; 

}  // namespace bustub
