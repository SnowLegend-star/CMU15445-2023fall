#include "concurrency/watermark.h"
#include <exception>
#include <limits>
#include "common/exception.h"
#include "storage/table/tuple.h"

namespace bustub {

auto Watermark::AddTxn(timestamp_t read_ts) -> void {
  if (read_ts < commit_ts_) {
    throw Exception("read ts < commit ts");
  }

  // TODO(fall2023): implement me!
  current_reads_[read_ts]++;
  read_ts_queue_.push(read_ts);
}

auto Watermark::RemoveTxn(timestamp_t read_ts) -> void {
  // TODO(fall2023): implement me!
  current_reads_[read_ts]--;
  while (!read_ts_queue_.empty()&&current_reads_[read_ts_queue_.top()]==0) {
    read_ts_queue_.pop();
  }
  // 如果没有read_ts了, 将WM置为默认值
  if(read_ts_queue_.empty()){
    watermark_=commit_ts_;
  }else{
    watermark_=read_ts_queue_.top();
  }
}

}  // namespace bustub
