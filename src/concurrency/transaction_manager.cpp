//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// transaction_manager.cpp
//
// Identification: src/concurrency/transaction_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "concurrency/transaction_manager.h"

#include <cstddef>
#include <memory>
#include <mutex>  // NOLINT
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "catalog/catalog.h"
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "common/rid.h"
#include "concurrency/transaction.h"
#include "execution/execution_common.h"
#include "storage/table/table_heap.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

auto TransactionManager::Begin(IsolationLevel isolation_level) -> Transaction * {
  std::unique_lock<std::shared_mutex> l(txn_map_mutex_);
  auto txn_id = next_txn_id_++;
  auto txn = std::make_unique<Transaction>(txn_id, isolation_level);
  auto *txn_ref = txn.get();
  txn_map_.insert(std::make_pair(txn_id, std::move(txn)));

  // TODO(fall2023): set the timestamps here. Watermark updated below.
  txn_ref->read_ts_=last_commit_ts_.load();

  running_txns_.AddTxn(txn_ref->read_ts_);
  return txn_ref;
}

auto TransactionManager::VerifyTxn(Transaction *txn) -> bool { return true; }

auto TransactionManager::Commit(Transaction *txn) -> bool {
  std::unique_lock<std::mutex> commit_lck(commit_mutex_);

  // TODO(fall2023): acquire commit ts!
  // last_commit_ts_.fetch_add(1);
  auto cur_commit_ts=last_commit_ts_+1; // 因为一次只能提交一个txn, 所以逻辑递增就可以了

  if (txn->state_ != TransactionState::RUNNING) { // 只有正在运行的txn才会出现commit这个行为
    throw Exception("txn not in running state");
  }

  if (txn->GetIsolationLevel() == IsolationLevel::SERIALIZABLE) {
    if (!VerifyTxn(txn)) {
      commit_lck.unlock();
      Abort(txn);
      return false;
    }
  }

  // TODO(fall2023): Implement the commit logic!

  std::unique_lock<std::shared_mutex> lck(txn_map_mutex_);
  

  // TODO(fall2023): set commit timestamp + update last committed timestamp here.
  last_commit_ts_=cur_commit_ts;
  txn->commit_ts_=cur_commit_ts;

  // 将当前txn写集包含的tuple全部更新
  auto write_sets=txn->GetWriteSets();
  for(const auto& elem:write_sets){
    auto table_id=elem.first;
    auto w_set=elem.second;
    auto cur_table_info=      catalog_->GetTable(table_id);
    for(const auto modified_rid:w_set){
      auto cur_tuple=cur_table_info->table_->GetTuple(modified_rid);
      cur_tuple.first.ts_=txn->commit_ts_;  //这个也需要更新, 不然不可以及时反映到base tuple上来
      auto is_delete=cur_tuple.first.is_deleted_;
      cur_table_info->table_->UpdateTupleMeta({txn->commit_ts_,is_delete}, modified_rid); // 这么更新才是对的
      auto undo_link_opt=GetUndoLink(modified_rid);
      if(undo_link_opt.has_value()){
        // 更新的本质是更新undolog的内容，而不是直接更新tuple
        auto undo_link=undo_link_opt.value();
        UndoLog new_undolog=txn->GetUndoLog(undo_link.prev_log_idx_);
        new_undolog.ts_=txn->commit_ts_;
        txn->ModifyUndoLog(undo_link.prev_log_idx_, new_undolog);
      }

      // 如果是insert, 在这里生成一个undolog
      if(txn->GetInsertState()){
        std::vector<bool> modified_field(cur_table_info->schema_.GetColumnCount(),true);
        auto new_link=txn->AppendUndoLog({cur_tuple.first.is_deleted_,modified_field,cur_tuple.second,txn->commit_ts_,{},true});
        UpdateUndoLink(modified_rid, new_link);
      }
    }
  }

  // 清空写集
  txn->write_set_.clear();

  txn->state_ = TransactionState::COMMITTED;
  running_txns_.UpdateCommitTs(txn->commit_ts_);
  running_txns_.RemoveTxn(txn->read_ts_);

  return true;
}

void TransactionManager::Abort(Transaction *txn) {
  if (txn->state_ != TransactionState::RUNNING && txn->state_ != TransactionState::TAINTED) {
    throw Exception("txn not in running / tainted state");
  }

  // TODO(fall2023): Implement the abort logic!

  std::unique_lock<std::shared_mutex> lck(txn_map_mutex_);
  txn->state_ = TransactionState::ABORTED;
  running_txns_.RemoveTxn(txn->read_ts_);
}

void TransactionManager::GarbageCollection() { 
  std::map<txn_id_t,size_t> txn_invisible_undolog; // 每个txn不可见的undoLog数量
  auto table_names=catalog_->GetTableNames();
  for(const auto & table_name:table_names){
    auto table_info=catalog_->GetTable(table_name);
    auto iter=table_info->table_->MakeIterator();

    // 开始遍历table中所有的tuple
    while(!iter.IsEnd()){
      // Tuple cur_tuple=iter.GetTuple();
      RID cur_rid=iter.GetRID();
      // 处理每个tuple对应的undologs
      auto undo_link_opt=GetUndoLink(cur_rid); // 每个tuple拥有的undoLog数量>=1
      if(undo_link_opt.has_value()){
        auto undo_link=undo_link_opt.value();
        while(undo_link.IsValid()){
          auto txn_id=undo_link.prev_txn_;
          // auto prev_log_idx=undo_link.prev_log_idx_;
          auto cur_txn=txn_map_[txn_id];
          auto undo_log_opt=GetUndoLogOptional(undo_link);
          if(undo_log_opt.has_value()){
            auto undo_log=undo_log_opt.value();
            // base_tuple不能算是undo_log
            if(undo_log.ts_<=GetWatermark()){
              txn_invisible_undolog[txn_id]++;
            }
            undo_link=undo_log.prev_version_;
          }             
        }
      }
      ++iter;
    }
  }

  // 开始删除txn
  // 注意这里不要边遍历map边进行删除
  std::vector<txn_id_t> deleted_txn;
  for(auto &txn_info:txn_map_){
    auto txn_id=txn_info.first;
    auto txn=txn_info.second.get();
    auto txn_state=txn->GetTransactionState();
    // 注意只有已经提交或者中止的txn可以被删除
    if(txn_invisible_undolog[txn_id]==txn->GetUndoLogNum()&&
      (txn_state==TransactionState::COMMITTED||txn_state==TransactionState::ABORTED)){
      deleted_txn.emplace_back(txn_id);
      txn->ClearUndoLog();
    }
  }

  for(auto const &id:deleted_txn){
    txn_map_.erase(id);
    std::cout<<"Txn: "<<id<<"被成功删除!"<<std::endl;
  }
}

}  // namespace bustub
