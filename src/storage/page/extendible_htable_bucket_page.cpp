//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_bucket_page.cpp
//
// Identification: src/storage/page/extendible_htable_bucket_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cstdint>
#include <optional>
#include <utility>

#include "common/exception.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/extendible_htable_bucket_page.h"

namespace bustub {

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::Init(uint32_t max_size) {
  size_=0;
  max_size_=max_size;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Lookup(const K &key, V &value, const KC &cmp) const -> bool {
  for(uint32_t i=0;i<size_;i++){
    if(cmp(array_[i].first,key)==0){
      value=array_[i].second;
        return true;
      }
    }
  return false;
}


// 没有重复的key
// 还没有实现分裂操作
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Insert(const K &key, const V &value, const KC &cmp) -> bool {
  // 如果这个bucket已经满了 
  if(size_==max_size_){ 
    return false;
  }

  // 如果有重复的key, 就地修改
  for(uint32_t i=0;i<size_;i++){
    if(cmp(array_[i].first,key)==0){
        array_[i].second=value;
        return true;
    }
  }

  // 在bucket的最末尾插入新的key
  array_[size_].first=key;
  array_[size_].second=value;
  size_++;
  return true;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Remove(const K &key, const KC &cmp) -> bool {
  uint32_t i = 0;
  for (; i < size_; ++i) {
    if (cmp(array_[i].first, key) == 0) {
      break;
    }
  }
  if (i == size_) {
    return false;
  }
  for (uint32_t j = i + 1; j < size_; ++j) {
    array_[j - 1].first = array_[j].first;
    array_[j - 1].second = array_[j].second;
  }
  size_--;
  return true;
}

// Attention Please!!!
// 这里的bucket_idx表示的是在array_的数组下标
// Directory中的bucket_idx表示的是bucket_page_id
// 两者很容易混淆
template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::RemoveAt(uint32_t bucket_idx) {
  // buk_idx不合法
  if(bucket_idx>=size_){
    return ;
  }
  uint32_t i=bucket_idx;
  for(;i<size_-1;i++){
    array_[i].first=array_[i+1].first;
    array_[i].second=array_[i+1].second;
  }
  size_--;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::KeyAt(uint32_t bucket_idx) const -> K {
  // buk_idx不合法
  if(bucket_idx>=size_){
    return {};
  }
  
  return array_[bucket_idx].first;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::ValueAt(uint32_t bucket_idx) const -> V {
    // buk_idx不合法
  if(bucket_idx>=size_){
    return {};
  }
  
  return array_[bucket_idx].second;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::EntryAt(uint32_t bucket_idx) const -> const std::pair<K, V> & {
  return array_[bucket_idx];
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Size() const -> uint32_t {
  return size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsFull() const -> bool {
  return size_==max_size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsEmpty() const -> bool {
  return size_==0;
}

template class ExtendibleHTableBucketPage<int, int, IntComparator>;
template class ExtendibleHTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
