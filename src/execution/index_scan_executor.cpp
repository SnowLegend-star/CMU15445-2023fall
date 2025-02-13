//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"
#include <vector>
#include "catalog/catalog.h"
#include "storage/index/extendible_hash_table_index.h"
#include "storage/table/tuple.h"
#include "type/value.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

// 通过索引找到相应的rid
void IndexScanExecutor::Init() {
  index_scan_finish_ = false;
  IndexInfo *index_info = GetExecutorContext()->GetCatalog()->GetIndex(plan_->index_oid_);
  auto htable = dynamic_cast<HashTableIndexForTwoIntegerColumn *>(index_info->index_.get());
  rids_.clear();
  // 获得索引的constant_value_expression 例如where v1=1, 这里val_就是1
  auto key_value = plan_->pred_key_->val_;
  std::vector<Value> values{key_value};
  Tuple key_tuple(values, &index_info->key_schema_);
  htable->ScanKey(key_tuple, &rids_, GetExecutorContext()->GetTransaction());
}

auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (index_scan_finish_) {
    return false;
  }
  index_scan_finish_ = true;
  if (rids_.empty()) {
    return false;
  }
  // 通过rid来找对应的元组
  TableInfo *table_info = GetExecutorContext()->GetCatalog()->GetTable(plan_->table_oid_);
  auto table_heap = table_info->table_.get();
  for (auto &tmp_rid : rids_) {  // 这里看似是for循环, 实则就只会找到一个符合条件的tuple
    auto meta = table_heap->GetTupleMeta(tmp_rid);
    if (!meta.is_deleted_) {
      *tuple = table_heap->GetTuple(tmp_rid).second;
      *rid = tmp_rid;
    }
  }
  return true;
}

}  // namespace bustub
