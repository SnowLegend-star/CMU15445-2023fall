//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.h
//
// Identification: src/include/storage/page/extendible_htable_directory_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * Directory page format:
 *  --------------------------------------------------------------------------------------
 * | MaxDepth (4) | GlobalDepth (4) | LocalDepths (512) | BucketPageIds(2048) | Free(1528)
 *  --------------------------------------------------------------------------------------
 */

#pragma once

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "common/config.h"
#include "storage/index/generic_key.h"

namespace bustub {

/**
 * HTABLE_DIRECTORY_PAGE_METADATA_SIZE 是目录页元数据的大小。
 * 它是 2 个 uint32_t 类型的大小，总共占用 8 字节。
 */
static constexpr uint64_t HTABLE_DIRECTORY_PAGE_METADATA_SIZE = sizeof(uint32_t) * 2;

/**
 * HTABLE_DIRECTORY_ARRAY_SIZE 是可扩展哈希索引中，目录页可以容纳的页面 ID 数量。
 * 这是 512，因为目录数组必须按 2 的幂增长，1024 个页面 ID 会导致无法为其他成员变量提供空间。
 */
static constexpr uint64_t HTABLE_DIRECTORY_MAX_DEPTH = 9;
static constexpr uint64_t HTABLE_DIRECTORY_ARRAY_SIZE = 1 << HTABLE_DIRECTORY_MAX_DEPTH;

/**
 * 扩展哈希表的目录页类。
 */
class ExtendibleHTableDirectoryPage {
 public:
  // 删除所有构造函数/析构函数以确保内存安全
  ExtendibleHTableDirectoryPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableDirectoryPage);

  /**
   * 在从缓冲池创建新的目录页后，必须调用初始化方法来设置默认值
   * @param max_depth 目录页的最大深度
   */
  void Init(uint32_t max_depth = HTABLE_DIRECTORY_MAX_DEPTH);

  /**
   * 获取哈希后的键对应的桶索引
   *
   * @param hash 键的哈希值
   * @return 哈希后的当前键对应的桶索引
   */
  auto HashToBucketIndex(uint32_t hash) const -> uint32_t;

  /**
   * 使用目录索引查找桶页面
   *
   * @param bucket_idx 目录中要查找的索引
   * @return 对应 bucket_idx 的桶页面 ID
   */
  auto GetBucketPageId(uint32_t bucket_idx) const -> page_id_t;

  /**
   * 使用桶索引和页面 ID 更新目录索引
   *
   * @param bucket_idx 目录索引，插入页面 ID 的位置
   * @param bucket_page_id 要插入的页面 ID
   */
  void SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id);

  /**
   * 获取某个索引的分裂映像
   *
   * @param bucket_idx 查找分裂映像的目录索引
   * @return 分裂映像的目录索引
   */
  auto GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t;

  /**
   * GetGlobalDepthMask - 返回一个 global_depth 位为 1，其余为 0 的掩码。
   *
   * 在扩展哈希中，我们使用以下哈希 + 掩码函数将键映射到目录索引：
   *
   * DirectoryIndex = Hash(key) & GLOBAL_DEPTH_MASK
   *
   * 其中，GLOBAL_DEPTH_MASK 是一个 32 位表示中，最低有效位（LSB）开始有 GLOBAL_DEPTH 个 1，其余为 0 的掩码。
   * 例如，global depth 为 3 时，对应的掩码是 0x00000007。
   *
   * @return global_depth 个 1 和其余位为 0 的掩码（最低有效位开始为 1）
   */
  auto GetGlobalDepthMask() const -> uint32_t{
  auto glo_depth=GetGlobalDepth();
  return (1<<(glo_depth))-1;
}

  /**
   * GetLocalDepthMask - 和 global depth 掩码相似，但它使用 bucket_idx 对应桶的局部深度（local depth）来计算。
   *
   * @param bucket_idx 查找局部深度的索引
   * @return 局部深度掩码，最低有效位开始为 1，其余位为 0
   */
  auto GetLocalDepthMask(uint32_t bucket_idx) const -> uint32_t{
  // 得到local_depth不就够了吗？这个函数真的是多此一举
  auto loc_depth=local_depths_[bucket_idx];
  return (1<<(loc_depth))-1;
}

  /**
   * 获取哈希表目录的全局深度
   *
   * @return 目录的全局深度
   */
  auto GetGlobalDepth() const -> uint32_t;

  auto GetMaxDepth() const -> uint32_t{
    return max_depth_;
  }

  /**
   * 增加目录的全局深度
   */
  void IncrGlobalDepth();

  /**
   * 减少目录的全局深度
   */
  void DecrGlobalDepth();

  /**
   * @return 如果目录可以缩小，则返回 true
   */
  auto CanShrink() -> bool;

  /**
   * @return 当前目录的大小
   */
  auto Size() const -> uint32_t;

  /**
   * @return 目录的最大大小
   */
  auto MaxSize() const -> uint32_t;

  /**
   * 获取桶在 bucket_idx 位置的局部深度
   *
   * @param bucket_idx 查找局部深度的桶索引
   * @return 对应桶的局部深度
   */
  auto GetLocalDepth(uint32_t bucket_idx) const -> uint32_t;

  /**
   * 设置桶在 bucket_idx 位置的局部深度为 local_depth
   *
   * @param bucket_idx 要更新的桶索引
   * @param local_depth 新的局部深度
   */
  void SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth);

  /**
   * 增加桶在 bucket_idx 位置的局部深度
   * @param bucket_idx 要增加局部深度的桶索引
   */
  void IncrLocalDepth(uint32_t bucket_idx);

  /**
   * 减少桶在 bucket_idx 位置的局部深度
   * @param bucket_idx 要减少局部深度的桶索引
   */
  void DecrLocalDepth(uint32_t bucket_idx);

  /**
   * 验证完整性
   *
   * 验证以下不变量：
   * (1) 所有局部深度 (LD) 必须小于等于全局深度 (GD)。
   * (2) 每个桶应该恰好有 2^(GD - LD) 个指向它的指针。
   * (3) 相同桶页面 ID 的所有局部深度 (LD) 应该相同。
   */
  void VerifyIntegrity() const;

  /**
   * 打印当前目录
   */
  void PrintDirectory() const;

 private:
  uint32_t max_depth_;  // 目录页的最大深度
  uint32_t global_depth_;  // 目录页的全局深度
  uint8_t local_depths_[HTABLE_DIRECTORY_ARRAY_SIZE];  // 存储每个桶的局部深度
  page_id_t bucket_page_ids_[HTABLE_DIRECTORY_ARRAY_SIZE];  // 存储每个桶的页面 ID
  uint32_t current_size_;  // 当前目录有效entry的数量
};
}  // namespace bustub
