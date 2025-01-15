//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// page.h
//
// Identification: src/include/storage/page/page.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstring>
#include <iostream>

#include "common/config.h"
#include "common/rwlatch.h"

namespace bustub {

/**
 * Page 是数据库系统中的基本存储单元。Page 提供了一个封装，表示实际的数据页，这些数据页存储在主内存中。Page
 * 还包含了一些用于缓冲池管理器的管理信息，例如：锁计数、脏页标志、页ID等。
 */
class Page {
  // Page 内部包含了一些只有缓冲池管理器才需要的管理信息。
  friend class BufferPoolManager;

 public:
  /** 构造函数。将页面数据清零。 */
  Page() {
    data_ = new char[BUSTUB_PAGE_SIZE];
    ResetMemory();
  }

  /** 默认析构函数。 */
  ~Page() { delete[] data_; }

  /** @return 页面中实际包含的数据 */
  inline auto GetData() -> char * { return data_; }

  /** @return 此页面的页ID */
  inline auto GetPageId() -> page_id_t { return page_id_; }

  /** @return 此页面的锁计数 */
  inline auto GetPinCount() -> int { return pin_count_; }

  /** @return 如果页面内容被修改（与磁盘上的版本不同），返回 true；否则返回 false */
  inline auto IsDirty() -> bool { return is_dirty_; }

  /** 获取页面的写锁。 */
  inline void WLatch() { rwlatch_.WLock(); }

  /** 释放页面的写锁。 */
  inline void WUnlatch() { rwlatch_.WUnlock(); }

  /** 获取页面的读锁。 */
  inline void RLatch() { rwlatch_.RLock(); }

  /** 释放页面的读锁。 */
  inline void RUnlatch() { rwlatch_.RUnlock(); }

  /** @return 页面日志序列号（LSN）。 */
  inline auto GetLSN() -> lsn_t { return *reinterpret_cast<lsn_t *>(GetData() + OFFSET_LSN); }

  /** 设置页面的日志序列号（LSN）。 */
  inline void SetLSN(lsn_t lsn) { memcpy(GetData() + OFFSET_LSN, &lsn, sizeof(lsn_t)); }

 protected:
  static_assert(sizeof(page_id_t) == 4);  // 页ID大小必须为4字节
  static_assert(sizeof(lsn_t) == 4);      // LSN大小必须为4字节

  static constexpr size_t SIZE_PAGE_HEADER = 8;   // 页面头部大小
  static constexpr size_t OFFSET_PAGE_START = 0;  // 页数据起始偏移量
  static constexpr size_t OFFSET_LSN = 4;         // LSN的偏移量

 private:
  /** 将页面内的数据清零。 */
  inline void ResetMemory() { memset(data_, OFFSET_PAGE_START, BUSTUB_PAGE_SIZE); }

  /** 页面内存中存储的实际数据。 */
  // 通常，这应该存储为 `char data_[BUSTUB_PAGE_SIZE]{};`。但为了让ASAN（AddressSanitizer）检测页面溢出，
  // 我们将其存储为指针类型。
  char *data_;

  /** 页的ID。 */
  page_id_t page_id_ = INVALID_PAGE_ID;

  /** 页的锁计数。 */
  int pin_count_ = 0;

  /** 页是否是脏页（即与磁盘上的页不同）。 */
  bool is_dirty_ = false;

  /** 页的读写锁。 */
  ReaderWriterLatch rwlatch_;
};

}  // namespace bustub
