//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.h
//
// Identification: src/include/container/disk/hash/extendible_hash_table.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <deque>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "concurrency/transaction.h"
#include "container/hash/hash_function.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

/**
 * 实现了一个扩展哈希表，基于缓冲池管理器进行支持。支持非唯一键。支持插入和删除操作。
 * 当桶变满或空时，表会动态增长/缩小。
 */
template <typename K, typename V, typename KC>
class DiskExtendibleHashTable {
 public:
  /**
   * @brief 创建一个新的扩展哈希表。
   *
   * @param name 表的名称
   * @param bpm 用于操作的缓冲池管理器
   * @param cmp 键的比较器
   * @param hash_fn 哈希函数
   * @param header_max_depth 头页允许的最大深度
   * @param directory_max_depth 目录页允许的最大深度
   * @param bucket_max_size 桶页数组的最大大小
   */
  explicit DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm, const KC &cmp,
                                   const HashFunction<K> &hash_fn, uint32_t header_max_depth = HTABLE_HEADER_MAX_DEPTH,
                                   uint32_t directory_max_depth = HTABLE_DIRECTORY_MAX_DEPTH,
                                   uint32_t bucket_max_size = HTableBucketArraySize(sizeof(std::pair<K, V>)));

  /** TODO(P2): 添加实现
   * 将一个键值对插入哈希表。
   *
   * @param key 要插入的键
   * @param value 与键关联的值
   * @param transaction 当前的事务
   * @return 如果插入成功则返回true，否则返回false
   */
  auto Insert(const K &key, const V &value, Transaction *transaction = nullptr) -> bool;

  /** TODO(P2): 添加实现
   * 从哈希表中删除一个键值对。
   *
   * @param key 要删除的键
   * @param value 要删除的值
   * @param transaction 当前的事务
   * @return 如果删除成功则返回true，否则返回false
   */
  auto Remove(const K &key, Transaction *transaction = nullptr) -> bool;

  /** TODO(P2): 添加实现
   * 获取与给定键关联的值。
   *
   * 注意(fall2023): 本学期你只需要支持唯一的键值对。
   *
   * @param key 要查找的键
   * @param[out] result 与给定键关联的值(s)
   * @param transaction 当前的事务
   * @return 返回与给定键关联的值(s)
   */
  auto GetValue(const K &key, std::vector<V> *result, Transaction *transaction = nullptr) const -> bool;

  /**
   * 帮助函数，用于验证扩展哈希表目录的完整性。
   */
  void VerifyIntegrity() const;

  /**
   * 帮助函数，用于暴露头页的页面ID。
   */
  auto GetHeaderPageId() const -> page_id_t;

  /**
   * 帮助函数，用于打印哈希表的内容。
   */
  void PrintHT() const;

 private:
  /**
   * 哈希 - 简单的帮助函数，用于将MurmurHash的64位哈希值下转为32位
   * 以便用于扩展哈希。
   *
   * @param key 要哈希的键
   * @return 下转为32位的哈希值
   */
  auto Hash(K key) const -> uint32_t;

// 这里给出hash真是令人费解？
// 好像是创建新目录的同时也要创建个新的Bucket吧
  auto InsertToNewDirectory(ExtendibleHTableHeaderPage *header, uint32_t directory_idx, uint32_t hash, const K &key,
                            const V &value) -> bool;

  auto InsertToNewBucket(ExtendibleHTableDirectoryPage *directory, uint32_t bucket_idx, const K &key, const V &value)
      -> bool;

// bucket分裂的时候就要更新上层的Directory
  void UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory, uint32_t new_bucket_idx,
                              page_id_t new_bucket_page_id, uint32_t new_local_depth, uint32_t local_depth_mask);

  void MigrateEntries(ExtendibleHTableBucketPage<K, V, KC> *old_bucket,
                      ExtendibleHTableBucketPage<K, V, KC> *new_bucket, uint32_t new_bucket_idx,
                      uint32_t local_depth_mask);

  // 成员变量
  std::string index_name_;  // 哈希表的名称
  BufferPoolManager *bpm_;  // 缓冲池管理器
  KC cmp_;  // 键的比较器
  HashFunction<K> hash_fn_;  // 哈希函数
  uint32_t header_max_depth_;  // 头页的最大深度
  uint32_t directory_max_depth_;  // 目录页的最大深度
  uint32_t bucket_max_size_;  // 桶页数组的最大大小
  page_id_t header_page_id_;  // 头页的页面ID
};

}  // namespace bustub
