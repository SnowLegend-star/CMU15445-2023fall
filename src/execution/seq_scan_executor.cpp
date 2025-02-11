//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"
#include "common/rid.h"
#include "storage/table/tuple.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan) : AbstractExecutor(exec_ctx) {
    plan_=plan;
}

void SeqScanExecutor::Init() {  
    table_info_=GetExecutorContext()->GetCatalog()->GetTable(plan_->GetTableOid());
    table_heap_=table_info_->table_.get();
    auto iter=table_heap_->MakeIterator();
    // 开始遍历这个table_heap
    rids_.clear();
    while(!iter.IsEnd()){
        rids_.push_back(iter.GetRID());
        ++iter;
    }
    iter_start_=rids_.begin();
}

// 如果没有遍历到table结束位置, 就一直寻找知道找到一个符合条件的tuple
auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool { 
    TupleMeta meta{};
    // auto iter=rids_.begin();    // 得将这个保存为static或者声明为成员变量
    do {
        if(iter_start_==rids_.end()){
            return false;
        }
        meta=table_heap_->GetTupleMeta(*iter_start_);
        if(!meta.is_deleted_){    // 这个tuple是有效的
            *tuple=table_heap_->GetTuple(*iter_start_).second;
            *rid=*iter_start_;
        }
        ++iter_start_;
    }while (meta.is_deleted_||(plan_->filter_predicate_!=nullptr&&!plan_->filter_predicate_->Evaluate(tuple, table_info_->schema_).GetAs<bool>()));
    return true;
}

}  // namespace bustub
