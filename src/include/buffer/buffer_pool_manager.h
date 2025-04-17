//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.h
//
// Identification: src/include/buffer/buffer_pool_manager.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <unordered_map>

#include "buffer/lru_k_replacer.h"
#include "common/config.h"
#include "recovery/log_manager.h"
#include "storage/disk/disk_scheduler.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"
#include "dlc.h"

namespace bustub {

/**
 * BufferPoolManager reads disk pages to and from its internal buffer pool.
 */
class BufferPoolManager {
 public:
  /**
   * @brief 创建一个新的 BufferPoolManager 实例。
   * @param pool_size 缓冲池的大小
   * @param disk_manager 磁盘管理器
   * @param replacer_k LRU-K 替换算法的常数 k
   * @param log_manager 日志管理器（仅用于测试：nullptr 表示禁用日志）。对于 P1 忽略该参数。
   */
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k = LRUK_REPLACER_K,
                    LogManager *log_manager = nullptr);

  /**
   * @brief Destroy an existing BufferPoolManager.
   */
  ~BufferPoolManager();

  /** @brief Return the size (number of frames) of the buffer pool. */
  auto GetPoolSize() -> size_t { return pool_size_; }

  /** @brief Return the pointer to all the pages in the buffer pool. */
  auto GetPages() -> Page * { return pages_; }

  /**
   * TODO(P1): 添加实现
   *
   * @brief 在缓冲池中创建一个新页面。设置 page_id 为新页面的 ID，或者如果当前所有帧都被使用且不可逐出（即被 pin
   * 住），则返回 nullptr。
   *
   * 你应该从空闲列表或替换器中选择一个替换帧（总是首先从空闲列表中选择），然后调用 AllocatePage() 方法来获得一个新页面
   * ID。如果替换的帧是脏页，你需要先将其写回磁盘。你还需要重置新页面的内存和元数据。
   *
   * 请记得通过调用 replacer.SetEvictable(frame_id, false) 将帧设置为 "Pin"，以防止替换器在缓冲池管理器 "Unpin"
   * 之前逐出它。 另外，记得在替换器中记录该帧的访问历史，以便 LRU-K 算法正常工作。
   *
   * @param[out] page_id 创建的页面 ID
   * @return 如果无法创建新页面，返回 nullptr，否则返回指向新页面的指针
   */
  auto NewPage(page_id_t *page_id) -> Page *;

  /**
   * TODO(P2): 添加实现
   *
   * @brief NewPage 的 PageGuard 封装。
   *
   * 功能应该与 NewPage 相同，不同之处在于
   * 它返回的是一个 BasicPageGuard 结构，而不是指向页面的指针。
   *
   * @param[out] page_id 新页面的 ID
   * @return 包含新页面的 BasicPageGuard
   */
  auto NewPageGuarded(page_id_t *page_id) -> BasicPageGuard;

  /**
   * TODO(P1): 添加实现
   *
   * @brief 从缓冲池中获取请求的页面。如果 page_id 需要从磁盘获取，但所有帧都在使用且不可逐出（即被 pin 住），则返回
   * nullptr。
   *
   * 首先在缓冲池中搜索
   * page_id。如果未找到，从空闲列表或替换器中选择一个替换帧（始终首先从空闲列表选择），通过调度一个读操作（DiskRequest）来从磁盘读取页面，并替换帧中的旧页面。与
   * NewPage 类似，如果旧页面是脏页，需要先将其写回磁盘，并更新新页面的元数据。
   *
   * 此外，记得禁用逐出操作并记录该帧的访问历史，就像在 NewPage 中一样。
   *
   * @param page_id 要获取的页面 ID
   * @param access_type 页面访问类型，仅在排行榜测试中使用
   * @return 如果无法获取 page_id，则返回 nullptr，否则返回请求页面的指针
   */
  auto FetchPage(page_id_t page_id, AccessType access_type = AccessType::Unknown) -> Page *;

  /**
   * TODO(P2): 添加实现
   *
   * @brief FetchPage 的 PageGuard 封装。
   *
   * 功能应该与 FetchPage 相同，不同之处在于
   * 依赖于所调用的函数，返回的页面已经持有相应的读锁或写锁。
   * 如果调用 FetchPageRead 或 FetchPageWrite，返回的页面应该已经持有相应的读锁或写锁。
   *
   * @param page_id 要获取的页面 ID
   * @return 包含获取页面的 PageGuard
   */
  auto FetchPageBasic(page_id_t page_id) -> BasicPageGuard;
  auto FetchPageRead(page_id_t page_id) -> ReadPageGuard;
  auto FetchPageWrite(page_id_t page_id) -> WritePageGuard;

  /**
   * TODO(P1): 添加实现
   *
   * @brief 解除目标页面在缓冲池中的 "Pin"。如果 page_id 不在缓冲池中或其 pin count 已经为 0，返回 false。
   *
   * 减少页面的 pin count。如果 pin count 达到 0，该帧应该可以被替换器逐出。
   * 另外，根据页面是否被修改，设置脏标志。
   *
   * @param page_id 要解除 pin 的页面 ID
   * @param is_dirty 如果页面被修改，则应标记为脏，反之则不标记
   * @param access_type 页面访问类型，仅在排行榜测试中使用
   * @return 如果页面不在页面表中或其 pin count <= 0，返回 false；否则返回 true
   */
  auto UnpinPage(page_id_t page_id, bool is_dirty, AccessType access_type = AccessType::Unknown) -> bool;

  /**
   * TODO(P1): 添加实现
   *
   * @brief 将目标页面刷新到磁盘。
   *
   * 使用 DiskManager::WritePage() 方法将页面刷新到磁盘，无论脏标志是否设置。
   * 刷新后，重置页面的脏标志。
   *
   * @param page_id 要刷新到磁盘的页面 ID，不能是 INVALID_PAGE_ID
   * @return 如果页面无法在页面表中找到，则返回 false，否则返回 true
   */
  auto FlushPage(page_id_t page_id) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Flush all the pages in the buffer pool to disk.
   */
  void FlushAllPages();

  /**
   * TODO(P1): 添加实现
   *
   * @brief 从缓冲池中删除页面。如果 page_id 不在缓冲池中，不执行任何操作并返回 true。如果页面被 pin
   * 住，无法删除，则立即返回 false。
   *
   * 从页面表中删除页面后，停止在替换器中跟踪该帧，并将帧重新加入空闲列表。同时，重置页面的内存和元数据。最后，调用
   * DeallocatePage() 来模拟释放磁盘上的页面。
   *
   * @param page_id 要删除的页面 ID
   * @return 如果页面存在但无法删除，返回 false；如果页面不存在或删除成功，返回 true
   */
  auto DeletePage(page_id_t page_id) -> bool;

 private:
  /** Number of pages in the buffer pool. */
  const size_t pool_size_;
  /** The next page id to be allocated  */
  std::atomic<page_id_t> next_page_id_ = 0;

  /** Array of buffer pool pages. */
  Page *pages_;
  /** Pointer to the disk sheduler. */
  std::unique_ptr<DiskScheduler> disk_scheduler_ __attribute__((__unused__));
  /** Pointer to the log manager. Please ignore this for P1. */
  LogManager *log_manager_ __attribute__((__unused__));
  /** Page table for keeping track of buffer pool pages. */
  std::unordered_map<page_id_t, frame_id_t> page_table_;
  /** Replacer to find unpinned pages for replacement. */
  std::unique_ptr<LRUKReplacer> replacer_;
  /** List of free frames that don't have any pages on them. */
  std::list<frame_id_t> free_list_;
  /** This latch protects shared data structures. We recommend updating this comment to describe what it protects. */
  std::mutex latch_;

  std::unique_ptr<DiskManagerProxy> disk_manager_proxy_; // 使用DiskManagerProxy代替原来的DiskScheduler

  /**
   * @brief 在磁盘上分配一个页面。调用者应该在调用此函数之前获取锁。
   * @return 分配的页面 ID
   */
  auto AllocatePage() -> page_id_t;

  /**
   * @brief 在磁盘上释放一个页面。调用者应该在调用此函数之前获取锁。
   * @param page_id 要释放的页面 ID
   */
  void DeallocatePage(__attribute__((unused)) page_id_t page_id) {
    // 目前此函数不执行任何操作，因为没有更复杂的数据结构来跟踪已释放的页面
  }

  // TODO(student): You may add additional private members and helper functions
};



}  // namespace bustub
