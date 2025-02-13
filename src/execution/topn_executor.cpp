#include "execution/executors/topn_executor.h"
#include <iostream>
#include "common/rid.h"
#include "execution/executors/sort_executor.h"
#include "storage/table/tuple.h"

namespace bustub {

TopNExecutor::TopNExecutor(ExecutorContext *exec_ctx, const TopNPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)),
      priority_heap_(Comparator(&plan_->OutputSchema(), plan_->order_bys_)) {}

void TopNExecutor::Init() {
  std::cout << plan_->ToString() << std::endl;
  child_executor_->Init();
  Tuple child_tuple{};
  RID child_rid{};
  while (child_executor_->Next(&child_tuple, &child_rid)) {
    priority_heap_.push(child_tuple);
    if (GetNumInHeap() > plan_->n_) {
      priority_heap_.pop();
    }
  }
  while (GetNumInHeap() != 0) {
    stack_.push(priority_heap_.top());
    priority_heap_.pop();
  }
}

auto TopNExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (!stack_.empty()) {
    *tuple = stack_.top();
    stack_.pop();
    return true;
  }
  return false;
}

auto TopNExecutor::GetNumInHeap() -> size_t { return priority_heap_.size(); };

}  // namespace bustub
