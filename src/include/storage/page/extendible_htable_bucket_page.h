//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_bucket_page.h
//
// Identification: src/include/storage/page/extendible_htable_bucket_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * Bucket page format:
 *  ----------------------------------------------------------------------------
 * | METADATA | KEY(1) + VALUE(1) | KEY(2) + VALUE(2) | ... | KEY(n) + VALUE(n)
 *  ----------------------------------------------------------------------------
 *
 * Metadata format (size in byte, 8 bytes in total):
 *  --------------------------------
 * | CurrentSize (4) | MaxSize (4)
 *  --------------------------------
 */
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/macros.h"
#include "storage/index/int_comparator.h"
#include "storage/page/b_plus_tree_page.h"
#include "type/value.h"

namespace bustub {

static constexpr uint64_t HTABLE_BUCKET_PAGE_METADATA_SIZE = sizeof(uint32_t) * 2;

constexpr auto HTableBucketArraySize(uint64_t mapping_type_size) -> uint64_t {
  return (BUSTUB_PAGE_SIZE - HTABLE_BUCKET_PAGE_METADATA_SIZE) / mapping_type_size;
};

/**
 * Bucket page for extendible hash table.
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
class ExtendibleHTableBucketPage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  ExtendibleHTableBucketPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableBucketPage);

  /**
  * 在从缓冲池创建新的桶页面后，必须调用初始化方法以设置默认值
  * @param max_size 桶数组的最大大小
  */
  void Init(uint32_t max_size = HTableBucketArraySize(sizeof(MappingType)));

  /**
  * 查找一个键
  *
  * @param key 要查找的键
  * @param[out] value 设置找到的值
  * @param cmp 比较器
  * @return 如果找到键和值，返回 true；如果没有找到，返回 false。
  */
  auto Lookup(const KeyType &key, ValueType &value, const KeyComparator &cmp) const -> bool;

  /**
  * 尝试将一个键值对插入到桶中。
  *
  * @param key 要插入的键
  * @param value 要插入的值
  * @param cmp 使用的比较器
  * @return 如果插入成功，返回 true；如果桶已满或键已存在，则返回 false
  */
  auto Insert(const KeyType &key, const ValueType &value, const KeyComparator &cmp) -> bool;

  /**
   * Removes a key and value.
   *
   * @return true if removed, false if not found
   */
  auto Remove(const KeyType &key, const KeyComparator &cmp) -> bool;

  void RemoveAt(uint32_t bucket_idx);

  /**
  * 获取桶中某个索引位置的键。
  *
  * @param bucket_idx 要获取键的桶中的索引
  * @return 桶中索引为 `bucket_idx` 位置的键
  */
  auto KeyAt(uint32_t bucket_idx) const -> KeyType;

  /**
  * 获取桶中某个索引位置的值。
  *
  * @param bucket_idx 要获取值的桶中的索引
  * @return 桶中索引为 `bucket_idx` 位置的值
  */
  auto ValueAt(uint32_t bucket_idx) const -> ValueType;

  /**
  * 获取桶中某个索引位置的条目。
  *
  * @param bucket_idx 要获取条目的桶中的索引
  * @return 桶中索引为 `bucket_idx` 位置的条目
  */
  auto EntryAt(uint32_t bucket_idx) const -> const std::pair<KeyType, ValueType> &;

  /**
   * @return number of entries in the bucket
   */
  auto Size() const -> uint32_t;

  /**
   * @return whether the bucket is full
   */
  auto IsFull() const -> bool;

  /**
   * @return whether the bucket is empty
   */
  auto IsEmpty() const -> bool;

  /**
   * Prints the bucket's occupancy information
   */
  void PrintBucket() const;

 private:
  uint32_t size_;
  uint32_t max_size_;
  MappingType array_[HTableBucketArraySize(sizeof(MappingType))];
};

}  // namespace bustub
