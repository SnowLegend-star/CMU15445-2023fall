#include "execution/execution_common.h"
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>
#include "catalog/catalog.h"
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "common/macros.h"
#include "common/rid.h"
#include "concurrency/transaction_manager.h"
#include "fmt/core.h"
#include "storage/table/table_heap.h"
#include "storage/table/tuple.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

auto GetUndoLogSchema(const Schema *base_schema, const std::vector<bool> &modified_fields)->Schema{
  auto col_cnt=base_schema->GetColumnCount();
  std::vector<Column> undolog_columns;
  for(uint32_t i=0;i<col_cnt;i++){
    if(modified_fields[i]){
      undolog_columns.emplace_back(base_schema->GetColumn(i));
    }
  }
  return Schema(undolog_columns);
}

  // 核心: 根据UndoLog里面的Partial Tuple Schema来将更改base_tuple 
  // 应用logs中最新的version，因为Table Heap只看得到最新版本
auto ReconstructTuple(const Schema *schema, const Tuple &base_tuple, const TupleMeta &base_meta,
                      const std::vector<UndoLog> &undo_logs) -> std::optional<Tuple> {
  // 只有最开始需要处理这种情况
  if(base_meta.is_deleted_&&undo_logs.empty()){
    return std::nullopt;
  }
  if(undo_logs.empty()){
    return base_tuple;
  }
  // 先获得undoLog的schema
  // 遍历undo_logs的顺序是从新到旧, 故这里从undo_logs尾部开始遍历
  auto cur_undolog=undo_logs[undo_logs.size()-1];
  if(cur_undolog.is_deleted_){
    return std::nullopt;    
  }
  auto base_col_cnt=schema->GetColumnCount();
  std::vector<Value> values;
  values.resize(base_col_cnt);
  std::vector<bool> modify_bitmap(base_col_cnt,false);
  uint32_t cnt=0;  // 用来记录bitmap被改写为true的数量
  // 从后往前遍历undoLog
  for(int j=undo_logs.size()-1;j>=0;j--){  // 这里j会一直大于0的
    cur_undolog=undo_logs[j];    
    Schema undolog_schema=GetUndoLogSchema(schema, cur_undolog.modified_fields_);
    uint32_t k=0; // 记录当前undolog的tuple下标
    for(uint32_t i=0;i<base_col_cnt;i++){
      if(cur_undolog.modified_fields_[i]){
        if(!modify_bitmap[i]){
          values[i]=cur_undolog.tuple_.GetValue(&undolog_schema, k);  
          modify_bitmap[i]=true;    
          cnt++;
          // 如果bitmap已经满了，提前返回
          if(cnt==base_col_cnt){
            Tuple new_tuple=Tuple{values,schema};
            return new_tuple; 
          }   
        }
        k++; 
      }
    }
  }
  // 到这里说明bitmap还没满
  // 用base tuple补齐values
  for(uint32_t i=0;i<base_col_cnt;i++){

    if(!modify_bitmap[i]){
      values[i]=base_tuple.GetValue(schema, i);
    }
  }
  Tuple new_tuple=Tuple{values,schema};
  return new_tuple;
}

void TxnMgrDbg(const std::string &info, TransactionManager *txn_mgr, const TableInfo *table_info,
               TableHeap *table_heap) {
  // always use stderr for printing logs...
  fmt::println(stderr, "debug_hook: {}", info);

  // fmt::println(
  //     stderr,
  //     "You see this line of text because you have not implemented `TxnMgrDbg`. You should do this once you have "
  //     "finished task 2. Implementing this helper function will save you a lot of time for debugging in later tasks.");

  // We recommend implementing this function as traversing the table heap and print the version chain. An example output
  // of our reference solution:
  //
  // debug_hook: before verify scan
  // RID=0/0 ts=txn8 tuple=(1, <NULL>, <NULL>)
  //   txn8@0 (2, _, _) ts=1
  // RID=0/1 ts=3 tuple=(3, <NULL>, <NULL>)
  //   txn5@0 <del> ts=2
  //   txn3@0 (4, <NULL>, <NULL>) ts=1
  // RID=0/2 ts=4 <del marker> tuple=(<NULL>, <NULL>, <NULL>)
  //   txn7@0 (5, <NULL>, <NULL>) ts=3
  // RID=0/3 ts=txn6 <del marker> tuple=(<NULL>, <NULL>, <NULL>)
  //   txn6@0 (6, <NULL>, <NULL>) ts=2
  //   txn3@1 (7, _, _) ts=1
  auto iter=table_heap->MakeIterator();
  while(!iter.IsEnd()){
    Tuple tmp_tuple=iter.GetTuple().second;
    RID tmp_rid=iter.GetRID();
    auto undo_link=txn_mgr->GetUndoLink(tmp_rid);

    std::vector<Value> values;  // 将这个tuple的内容打印出来
    for(uint32_t col=0;col<table_info->schema_.GetColumnCount();col++){
      values.emplace_back(tmp_tuple.GetValue(&table_info->schema_, col));
    }
    // 这个ts可能是临时的TXN_START_ID + txn_human_readable_id = txn_id, 有可能是已经完成提交的
    auto ts=iter.GetTuple().first.ts_;
    auto ts_real=ts>TXN_START_ID?"txn"+std::to_string(ts-TXN_START_ID):std::to_string(ts);
    fmt::println(stderr,"RID={}/{} ts={} tuple={}",tmp_rid.GetPageId(),tmp_rid.GetSlotNum(),ts_real,tmp_tuple.ToString(&table_info->schema_));
    // fmt::println("这个元组的undoLog如下:");

    while (undo_link->IsValid()) {
      if (undo_link.has_value()) {
        auto undo_logo = txn_mgr->GetUndoLogOptional(undo_link.value());
        if (undo_logo.has_value()) {
          Schema schema = GetUndoLogSchema(&table_info->schema_,undo_logo->modified_fields_ );
          fmt::println(stderr, "-------txn_id{} {} ts={} Del={}", undo_link.value().prev_txn_ - TXN_START_ID,
                       undo_logo->tuple_.ToString(&schema), undo_logo->ts_, undo_logo->is_deleted_);
          undo_link = undo_logo->prev_version_;
        } else {
          // undo_log已经遍历到结尾了
          break;
        }
      } else {
        // 一般不会出现这种情况
        fmt::println(stderr, "undo link has no value");
        break;
      }
    }
    ++iter;
  }
}

}  // namespace bustub
