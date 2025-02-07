//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "common/rid.h"
#include "execution/executor_context.h"
#include "execution/executors/insert_executor.h"
#include "storage/table/table_heap.h"
#include "storage/table/tuple.h"
#include "type/type.h"
#include "type/type_id.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
        plan_=plan;
        child_executor_=std::move(child_executor);
}

void InsertExecutor::Init() { 
    child_executor_->Init();
    insert_finish_=false;
}

auto InsertExecutor::Next( Tuple *tuple, RID *rid) -> bool { 
    // 防止重复插入
    if(insert_finish_){
        return false;
    }
    insert_finish_=true;
    TableInfo *table_info=GetExecutorContext()->GetCatalog()->GetTable(plan_->GetTableOid());
    auto schema=table_info->schema_;
    auto table_indexes=GetExecutorContext()->GetCatalog()->GetTableIndexes(table_info->name_);
    int cnt=0;  // 待插入的tuple数
    while (child_executor_->Next(tuple, rid)) {
        cnt++;
        std::optional<RID> new_rid_optional=table_info->table_->InsertTuple(TupleMeta{0,false}, *tuple);
        // 更新索引
        RID new_rid=new_rid_optional.value();
        for(auto index_info:table_indexes){
            auto key=tuple->KeyFromTuple(schema, index_info->key_schema_, index_info->index_->GetKeyAttrs());
            index_info->index_->InsertEntry(key, new_rid, exec_ctx_->GetTransaction()); // 可以更新索引吗
        }
    }
    // 构造输出table
    std::vector<Value> res={{TypeId::INTEGER,cnt}};
    *tuple=Tuple(res,&GetOutputSchema());
    return true;
}

}  // namespace bustub
