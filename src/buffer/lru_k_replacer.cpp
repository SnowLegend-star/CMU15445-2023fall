//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <iostream>
#include <memory>
#include <utility>
#include "common/macros.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool{
  std::scoped_lock<std::mutex> lock(latch_);
  
  size_t largest_bkd = 0;
  size_t largest_distance = 0;
  bool success_evict = false;
  bool is_inf = false;
  LRUKNode *evict_node_ptr = nullptr;

  // 优化：遍历 node_store_，选择 evict 的节点
  for (auto &pair : node_store_) {
      auto &node = pair.second;
      if (!node.is_evictable_) {
          continue;
      }
      success_evict = true;

      // 优化：空历史记录的节点可以直接驱逐
      if (node.history_.empty()) {
          *frame_id = pair.first;
          evict_node_ptr = &node;
          break;
      }

      // 优化：处理有足够历史记录的节点
      if (node.history_.size() == k_ && !is_inf) {
          auto current_bkd = current_timestamp_ - node.GetBackKTimeStamp();
          if (current_bkd > largest_bkd) {
              largest_bkd = current_bkd;
              *frame_id = pair.first;
              evict_node_ptr = &node;
          }
      } else if (node.history_.size() < k_) {
          // 优化：处理历史记录较少的节点
          is_inf = true;
          auto current_distance = current_timestamp_ - node.GetLatestTimeStamp();
          if (current_distance > largest_distance) {
              largest_distance = current_distance;
              *frame_id = pair.first;
              evict_node_ptr = &node;
          }
      }
  }

  // 优化：驱逐成功后清理
  if (success_evict) {
      current_size_--;
      evict_node_ptr->history_.clear();
      node_store_.erase(*frame_id);
  }

  return success_evict;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, AccessType access_type) {
  std::scoped_lock<std::mutex> lock(latch_);
  auto helper = static_cast<size_t>(frame_id);
  BUSTUB_ASSERT(helper <= replacer_size_, "invalid frame_id");

  auto iter = node_store_.find(frame_id);
  if (iter == node_store_.end()) {
      // 优化：如果没找到，直接创建并插入新节点
      auto new_node_ptr = std::make_unique<LRUKNode>();
      if (access_type != AccessType::Scan) {
          new_node_ptr->history_.push_back(current_timestamp_++);
      }
      node_store_.insert({frame_id, *new_node_ptr});
  } else {
      auto &node = iter->second;
      if (access_type != AccessType::Scan) {
          if (node.history_.size() == k_) {
              node.history_.pop_front();
          }
          node.history_.push_back(current_timestamp_++);
      }
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::scoped_lock<std::mutex> lock(latch_);
  auto helper = static_cast<size_t>(frame_id);
  BUSTUB_ASSERT(helper <= replacer_size_, "invalid frame_id");

  auto iter = node_store_.find(frame_id);
  if (iter == node_store_.end()) {
      // 如果没有找到，创建新节点
      node_store_.insert({frame_id, LRUKNode()});
  }
  auto &node = node_store_.at(frame_id);
  if (set_evictable && !node.is_evictable_) {
      node.is_evictable_ = true;
      current_size_++;
  }
  if (!set_evictable && node.is_evictable_) {
      node.is_evictable_ = false;
      current_size_--;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);
  auto iter = node_store_.find(frame_id);
  if (iter == node_store_.end()) {
      return;
  }
  auto &node = iter->second;
  BUSTUB_ASSERT(node.is_evictable_, "Called on a non-evictable frame.");

  node.history_.clear();
  node_store_.erase(frame_id);
  current_size_--;
}


auto LRUKReplacer::Size() -> size_t { return current_size_; }

}  // namespace bustub