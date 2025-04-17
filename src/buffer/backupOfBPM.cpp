#include <cassert>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include "buffer/buffer_pool_manager.h"

#include "buffer/dlc.h"
#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(disk_manager)), log_manager_(log_manager) {
  // TODO(students): remove this line after you have implemented the buffer pool manager
  // throw NotImplementedException(
  //     "BufferPoolManager is not implemented yet. If you have finished implementing BPM, please remove the throw "
  //     "exception line in buffer_pool_manager.cpp.");

  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
  std::cout << "用来初始化BufferPoolManager的参数为: ======== (pool_size: " << pool_size
            << " | replacer_k: " << replacer_k << ") ========" << std::endl;
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  // 加把大锁
  std::scoped_lock<std::mutex> lock(latch_);
  frame_id_t replacement_frame_id;
  if (!free_list_.empty()) {
    replacement_frame_id = free_list_.front();
    free_list_.pop_front();
  } else {  // 没有空的frame, 需要进行evict

    if (!replacer_->Evict(&replacement_frame_id)) {
      page_id = nullptr;
      return nullptr;
    }
    auto &helper = pages_[replacement_frame_id];
    if (helper.IsDirty()) {
      auto promise = disk_scheduler_->CreatePromise();
      auto future = promise.get_future();
      disk_scheduler_->Schedule({true, helper.GetData(), helper.page_id_, std::move(promise)});
      future.get();
    }
    page_table_.erase(helper.page_id_);
  }
  // 分配并初始化这个新的page
  auto new_page_id = AllocatePage();
  *page_id = new_page_id;
  pages_[replacement_frame_id].ResetMemory();
  pages_[replacement_frame_id].pin_count_ = 0;
  pages_[replacement_frame_id].is_dirty_ = false;
  page_table_.insert(std::make_pair(new_page_id, replacement_frame_id));
  replacer_->SetEvictable(replacement_frame_id, false);  // 分配的page默认为不可evict
  replacer_->RecordAccess(replacement_frame_id);         // 默认被访问一次
  pages_[replacement_frame_id].pin_count_++;
  pages_[replacement_frame_id].page_id_ = new_page_id;
  return &pages_[replacement_frame_id];
}

auto BufferPoolManager::FetchPage(page_id_t page_id, AccessType access_type) -> Page * {
  std::scoped_lock<std::mutex> lock(latch_);
  auto iter = page_table_.find(page_id);
  if (iter != page_table_.end()) {
    auto frame_id = iter->second;
    replacer_->SetEvictable(frame_id, false);
    replacer_->RecordAccess(frame_id, access_type);
    pages_[iter->second].pin_count_++;
    return &pages_[iter->second];
  }

  // 没能在页表找到这个page
  frame_id_t replacement_frame_id;
  if (!free_list_.empty()) {
    replacement_frame_id = free_list_.front();
    free_list_.pop_front();
  } else {  //没有多余的frame可供分配
    if (!replacer_->Evict(&replacement_frame_id)) {
      return nullptr;
    }
    auto &helper = pages_[replacement_frame_id];
    if (helper.IsDirty()) {
      auto promise1 = disk_scheduler_->CreatePromise();
      auto future1 = promise1.get_future();
      disk_scheduler_->Schedule({true, helper.data_, helper.page_id_, std::move(promise1)});
      future1.get();
    }
    page_table_.erase(helper.page_id_);
  }
  // 初始化这个page
  pages_[replacement_frame_id].ResetMemory();
  pages_[replacement_frame_id].pin_count_ = 0;
  pages_[replacement_frame_id].is_dirty_ = false;
  auto promise2 = disk_scheduler_->CreatePromise();
  auto future2 = promise2.get_future();
  disk_scheduler_->Schedule({false, pages_[replacement_frame_id].GetData(), page_id, std::move(promise2)});
  page_table_.insert(std::make_pair(page_id, replacement_frame_id));
  replacer_->SetEvictable(replacement_frame_id, false);
  replacer_->RecordAccess(replacement_frame_id, access_type);
  pages_[replacement_frame_id].pin_count_++;
  pages_[replacement_frame_id].page_id_ = page_id;

  future2.get();  // 这句话是必须的，否则可能没写完数据这个程序就直接运行结束了!!

  return &pages_[replacement_frame_id];
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
  BUSTUB_ASSERT(page_id != INVALID_PAGE_ID, "page_id is invalid");
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return false;
  }
  auto w_frame_id = it->second;
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  disk_scheduler_->Schedule({true, pages_[w_frame_id].data_, pages_[w_frame_id].page_id_, std::move(promise)});
  future.get();
  pages_[w_frame_id].is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock<std::mutex> lock(latch_);
  for (auto &pair : page_table_) {
    page_id_t page_id = pair.first;
    BUSTUB_ASSERT(page_id != INVALID_PAGE_ID, "page_id is invalid");
    auto iter = page_table_.find(page_id);
    auto target_id = iter->second;
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, pages_[target_id].data_, pages_[target_id].page_id_, std::move(promise)});
    future.get();
    pages_[target_id].is_dirty_ = false;
  }
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