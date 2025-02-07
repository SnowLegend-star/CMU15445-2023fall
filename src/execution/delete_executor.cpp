//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include <utility>
#include <vector>

#include "catalog/catalog.h"
#include "common/rid.h"
#include "execution/executors/delete_executor.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),plan_(plan),child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() { 
    child_executor_->Init();
    delete_finish_=false;
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool { 

    if(delete_finish_){
        return false;
    }
    delete_finish_=true;
    TableInfo *table_info=GetExecutorContext()->GetCatalog()->GetTable(plan_->table_oid_);
    auto table_indexes=GetExecutorContext()->GetCatalog()->GetTableIndexes(table_info->name_);
    int cnt=0;  // 对删除的行数进行计数
    Tuple tmp_tuple{};
    RID tmp_rid{};
    while(child_executor_->Next(&tmp_tuple, &tmp_rid)){
        cnt++;
        table_info->table_->UpdateTupleMeta(TupleMeta{0,true}, tmp_rid);
        for(auto & index:table_indexes){
            auto old_key=tmp_tuple.KeyFromTuple(table_info->schema_, index->key_schema_, index->index_->GetKeyAttrs());
            index->index_->DeleteEntry(old_key, tmp_rid, GetExecutorContext()->GetTransaction());
        }
    }
    std::vector<Value> res={{TypeId::INTEGER,cnt}};
    *tuple=Tuple{res,&GetOutputSchema()};
    return true;
}

}  // namespace bustub
