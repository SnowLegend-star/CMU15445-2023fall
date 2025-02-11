//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include <cstdint>
#include <utility>
#include <vector>
#include "binder/table_ref/bound_join_ref.h"
#include "common/rid.h"
#include "storage/table/tuple.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_child)),
      right_executor_(std::move(right_child)) {
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2023 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void HashJoinExecutor::Init() {
  std::cout << plan_->ToString() << std::endl;
  // std::cout<<plan_->GetLeftPlan()->ToString()<<std::endl;
  // std::cout<<plan_->GetRightPlan()->ToString()<<std::endl;
  left_executor_->Init();
  right_executor_->Init();
  Tuple child_tuple{};
  RID child_rid{};
  // 对于内连接, left table或者right table都可以作为哈希表
  // 但是对于做连接, 只能用right table作为哈希表
  while (right_executor_->Next(&child_tuple, &child_rid)) {
    auto hash_key = MakeRightTupleHashKey(&child_tuple);
    hashtable_.InsertKey(hash_key, child_tuple);
  }
  tuple_match_ = false;
  // 提取left table的第一个元素进行初始化哈希表
  left_status_ = left_executor_->Next(&left_cur_tuple_, &child_rid);
  if(left_status_){
    // 在哈希表中寻找left_tuple对应的key
    auto left_tuple_hash_key = MakeLeftTupleHashKey(&left_cur_tuple_);
    correspond_value_ = hashtable_.GetValue(left_tuple_hash_key).hashjoin_;
    iter_=correspond_value_.begin();
  }
}

// 遍历right table, 从而与哈希表的内容进行比对
auto HashJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while(true){
    if (!left_status_) {  // 如果left table遍历结束, 直接返回
      return false;
    }
    Schema left_schema = plan_->GetLeftPlan()->OutputSchema();
    Schema right_schema = plan_->GetRightPlan()->OutputSchema();
    // std::cout<<correspond_value_.size()<<std::endl;
    if (correspond_value_.empty()) {
      tuple_match_=false;
      // 对于左连接的特殊处理
      if (plan_->join_type_ == JoinType::LEFT) {
        std::vector<Value> result;
        for (uint32_t i = 0; i < left_schema.GetColumnCount(); i++) {
          result.emplace_back(left_cur_tuple_.GetValue(&left_schema, i));
        }
        for (uint32_t i = 0; i < right_schema.GetColumnCount(); i++) {
          result.emplace_back(ValueFactory::GetNullValueByType(right_schema.GetColumn(i).GetType()));
        }
        *tuple = Tuple{result, &plan_->OutputSchema()};
        left_status_=left_executor_->Next(&left_cur_tuple_, rid);
        // if (!left_status_) {  // 这里不可以加这个判断, 否则会导致提前返回
        //   return false;
        // }
        // 在哈希表中寻找left_tuple对应的key
        auto left_tuple_hash_key = MakeLeftTupleHashKey(&left_cur_tuple_);
        correspond_value_ = hashtable_.GetValue(left_tuple_hash_key).hashjoin_;
        iter_=correspond_value_.begin();

        return true;    
      }
      // 对于inner join的处理
      left_status_=left_executor_->Next(&left_cur_tuple_, rid);
      if (!left_status_) {  
          return false;
      }
      auto left_tuple_hash_key = MakeLeftTupleHashKey(&left_cur_tuple_);
      correspond_value_ = hashtable_.GetValue(left_tuple_hash_key).hashjoin_;
      iter_=correspond_value_.begin();
    }
    // left_tuple成功在哈希表中找到对应的key
    // 对应value中可能有多个right table的元组
    if(iter_!=correspond_value_.end()){
      Tuple right_tuple=*iter_;
      std::vector<Value> result;
      for (uint32_t i = 0; i < left_schema.GetColumnCount(); i++) {
        result.emplace_back(left_cur_tuple_.GetValue(&left_schema, i));
      }
      for (uint32_t i = 0; i < right_schema.GetColumnCount(); i++) {
        result.emplace_back(right_tuple.GetValue(&right_schema, i));
      }
      *tuple = Tuple{result, &plan_->OutputSchema()};
      iter_++;
      tuple_match_=true;
      return true;
    }
    // 如果右表对应的tuples的元素已经遍历完了, 说明没有与这个left tuple元素匹配的right tuple了
    // 现在要区分是left tuple到底事先有没有和right tuple匹配上
    if(tuple_match_){ // 这种情况是两者已经匹配上了, 进入left table的下一个tuple匹配
      left_status_=left_executor_->Next(&left_cur_tuple_, rid);
      if (!left_status_) {  // 如果left table遍历结束, 直接返回
        return false;
      }
      // 在哈希表中寻找left_tuple对应的key
      auto left_tuple_hash_key = MakeLeftTupleHashKey(&left_cur_tuple_);
      correspond_value_ = hashtable_.GetValue(left_tuple_hash_key).hashjoin_;
      iter_=correspond_value_.begin();
    }
  }

}

}  // namespace bustub
