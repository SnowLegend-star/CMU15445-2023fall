//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.h
//
// Identification: src/include/storage/page/extendible_htable_header_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * Header page format:
 *  ---------------------------------------------------
 * | DirectoryPageIds(2048) | MaxDepth (4) | Free(2044)
 *  ---------------------------------------------------
 */

#pragma once

#include <cstdlib>
#include "common/config.h"
#include "common/macros.h"

namespace bustub {

// 以下是一些常量的定义：

static constexpr uint64_t HTABLE_HEADER_PAGE_METADATA_SIZE = sizeof(uint32_t);  // 哈希表头页面的元数据大小（一个 uint32_t 的大小）
static constexpr uint64_t HTABLE_HEADER_MAX_DEPTH = 9;  // 哈希表头页面的最大深度（最大深度为 9）
static constexpr uint64_t HTABLE_HEADER_ARRAY_SIZE = 1 << HTABLE_HEADER_MAX_DEPTH;  // 哈希表头页面中目录数组的大小，基于最大深度（2^9）

// 哈希表头页面类的定义
class ExtendibleHTableHeaderPage {
 public:
  // 删除所有构造函数和析构函数以确保内存安全
  ExtendibleHTableHeaderPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableHeaderPage);

  /**
   * 创建新的头页面后，必须调用初始化方法来设置默认值
   * @param max_depth 头页面的最大深度
   */
  void Init(uint32_t max_depth = HTABLE_HEADER_MAX_DEPTH);

  /**
   * Get the directory index that the key is hashed to
   *
   * @param hash 键的哈希值
   * @return 哈希后的目录索引
   */
  auto HashToDirectoryIndex(uint32_t hash) const -> uint32_t;

  /**
   * 获取给定索引位置的目录页面 ID
   *
   * @param directory_idx 目录数组的索引
   * @return 索引位置的目录页面 ID
   */
  auto GetDirectoryPageId(uint32_t directory_idx) const -> uint32_t;

  /**
   * 设置目录数组中某个索引位置的目录页面 ID
   *
   * @param directory_idx 目录数组的索引
   * @param directory_page_id 目录页面的 ID
   */
  void SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id);

  /**
   * 获取头页面能够处理的最大目录页面 ID 数量
   */
  auto MaxSize() const -> uint32_t;

  /**
   * 打印头页面的占用信息
   */
  void PrintHeader() const;

 private:
  // 目录页面 ID 数组，大小为 HTABLE_HEADER_ARRAY_SIZE
  page_id_t directory_page_ids_[HTABLE_HEADER_ARRAY_SIZE];
  
  // 最大深度
  uint32_t max_depth_;
};

// 检查 page_id_t 的大小是否为 4 字节
static_assert(sizeof(page_id_t) == 4);

// 检查 ExtendibleHTableHeaderPage 的大小是否符合预期
static_assert(sizeof(ExtendibleHTableHeaderPage) == 
              sizeof(page_id_t) * HTABLE_HEADER_ARRAY_SIZE + HTABLE_HEADER_PAGE_METADATA_SIZE);

// 检查 ExtendibleHTableHeaderPage 的大小是否不超过页面大小
static_assert(sizeof(ExtendibleHTableHeaderPage) <= BUSTUB_PAGE_SIZE);

}  // namespace bustub