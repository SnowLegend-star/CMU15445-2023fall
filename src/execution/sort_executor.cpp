#include "execution/executors/sort_executor.h"
#include <algorithm>
#include <vector>
#include "common/rid.h"

namespace bustub {

SortExecutor::SortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

// static void Compare(const Tuple &tuple1, const Tuple &tuple2){
// }

void SortExecutor::Init() {
  child_executor_->Init();
  Tuple child_tuple;
  RID child_rid;
  while (child_executor_->Next(&child_tuple, &child_rid)) {
    tuples_.emplace_back(child_tuple);
  }
  std::sort(tuples_.begin(), tuples_.end(), Comparator(&GetOutputSchema(), plan_->GetOrderBy()));
  iter_ = tuples_.begin();
}

auto SortExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (iter_ == tuples_.end()) {
    return false;
  }
  *tuple = *iter_;
  ++iter_;
  return true;
}

}  // namespace bustub
