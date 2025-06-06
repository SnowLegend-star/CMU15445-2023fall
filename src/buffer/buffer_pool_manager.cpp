//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

#include "buffer/dlc.h"
#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"


namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(disk_manager)), log_manager_(log_manager),
    disk_manager_proxy_(std::make_unique<DiskManagerProxy>(disk_manager))
    {
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
  std::cout << "用来初始化BufferPoolManager的参数为: ======== (pool_size: " << pool_size
            << " | replacer_k: " << replacer_k << ") ========" << std::endl;
}

BufferPoolManager::~BufferPoolManager() { disk_manager_proxy_->Shutdown();
  delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::scoped_lock<std::mutex> lock(latch_);
  frame_id_t replacement_frame_id;

  if (!free_list_.empty()) {
    replacement_frame_id = free_list_.front();
    free_list_.pop_front();
  } else {
    if (!replacer_->Evict(&replacement_frame_id)) {
      *page_id = INVALID_PAGE_ID;
      return nullptr;
    }
    auto &victim_page = pages_[replacement_frame_id];
    if (victim_page.IsDirty()) {
      // 这里改用DiskManagerProxy异步写入，不再等待
      disk_manager_proxy_->WritePage(victim_page.page_id_, victim_page.data_);
    }
    page_table_.erase(victim_page.page_id_);
  }

  auto new_page_id = AllocatePage();
  *page_id = new_page_id;

  auto &new_page = pages_[replacement_frame_id];
  new_page.ResetMemory();
  new_page.pin_count_ = 1;
  new_page.is_dirty_ = false;
  new_page.page_id_ = new_page_id;

  page_table_[new_page_id] = replacement_frame_id;
  replacer_->SetEvictable(replacement_frame_id, false);
  replacer_->RecordAccess(replacement_frame_id, AccessType::Unknown);

  return &new_page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id, AccessType access_type) -> Page * {
  std::scoped_lock<std::mutex> lock(latch_);
  auto iter = page_table_.find(page_id);
  if (iter != page_table_.end()) {
    auto frame_id = iter->second;
    replacer_->SetEvictable(frame_id, false);
    replacer_->RecordAccess(frame_id, access_type);
    pages_[frame_id].pin_count_++;
    return &pages_[frame_id];
  }

  frame_id_t replacement_frame_id;
  if (!free_list_.empty()) {
    replacement_frame_id = free_list_.front();
    free_list_.pop_front();
  } else {
    if (!replacer_->Evict(&replacement_frame_id)) {
      return nullptr;
    }
    auto &victim_page = pages_[replacement_frame_id];
    if (victim_page.IsDirty()) {
      // 异步写回，不再等待磁盘写入
      disk_manager_proxy_->WritePage(victim_page.page_id_, victim_page.data_);
    }
    page_table_.erase(victim_page.page_id_);
  }

  auto &new_page = pages_[replacement_frame_id];
  new_page.ResetMemory();
  new_page.pin_count_ = 1;
  new_page.is_dirty_ = false;
  new_page.page_id_ = page_id;

  page_table_[page_id] = replacement_frame_id;
  replacer_->SetEvictable(replacement_frame_id, false);
  replacer_->RecordAccess(replacement_frame_id, access_type);

  // 从磁盘中同步读取需要的数据（读取仍需同步等待）
  disk_manager_proxy_->ReadPage(page_id, new_page.data_);

  return &new_page;
}


auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  auto it = page_table_.find(page_id);
  if (it == page_table_.end() || pages_[it->second].pin_count_ <= 0) {
    return false;
  }
  pages_[it->second].pin_count_--;
  if (pages_[it->second].pin_count_ == 0) {
    replacer_->SetEvictable(it->second, true);
  }
  if (is_dirty) {
    pages_[it->second].is_dirty_ = is_dirty;
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) { 
    return false;
  }

  disk_manager_proxy_->WritePage(page_id, pages_[it->second].data_);
  return true; // 异步提交刷新请求即可
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock<std::mutex> lock(latch_);
  for (auto &[page_id, frame_id] : page_table_) {
    disk_manager_proxy_->WritePage(page_id, pages_[frame_id].data_);
  }
  // 若需要保证数据完全落盘，DiskManagerProxy可提供Flush接口同步等待
}


auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return true;
  }
  auto target_frame_id = it->second;
  if (pages_[target_frame_id].pin_count_ > 0) {
    return false;
  }
  // 在相关数据结构清楚这个pg的信息
  page_table_.erase(page_id);
  replacer_->Remove(target_frame_id);
  free_list_.push_back(target_frame_id);
  pages_[target_frame_id].ResetMemory();

  // 初始化这个page的metadata
  pages_[target_frame_id].page_id_ = INVALID_PAGE_ID;
  pages_[target_frame_id].pin_count_ = 0;
  pages_[target_frame_id].is_dirty_ = false;

  DeallocatePage(page_id);
  std::cout << "成功删除page_id: " << page_id << std::endl;
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  auto page_basic_ptr = FetchPage(page_id);
  return BasicPageGuard{this, page_basic_ptr};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto page_read_ptr = FetchPage(page_id);
  page_read_ptr->RLatch();
  assert(page_read_ptr != nullptr);
  return ReadPageGuard{this, page_read_ptr};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto page_write_ptr = FetchPage(page_id);
  page_write_ptr->WLatch();
  // std::cout << "page_id: " << page_id << "获得写锁" << std::endl;
  assert(page_write_ptr != nullptr);
  return WritePageGuard{this, page_write_ptr};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  auto pg_ptr = NewPage(page_id);
  assert(pg_ptr != nullptr);
  return BasicPageGuard{this, pg_ptr};
}

}  // namespace bustub
