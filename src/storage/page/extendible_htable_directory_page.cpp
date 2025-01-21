//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.cpp
//
// Identification: src/storage/page/extendible_htable_directory_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_directory_page.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <unordered_map>

#include "common/config.h"
#include "common/logger.h"
#include "storage/page/hash_table_directory_page.h"

namespace bustub {

void ExtendibleHTableDirectoryPage::Init(uint32_t max_depth) {
  max_depth_=max_depth;
  global_depth_=0;
  current_size_=0;
  for(auto &elem:local_depths_){
    elem=0;
  }
  for(auto &buk_page_id:bucket_page_ids_){
    buk_page_id=INVALID_PAGE_ID;
  }
}

// 这个函数和Header部分的也是一样的吧
auto ExtendibleHTableDirectoryPage::HashToBucketIndex(uint32_t hash) const -> uint32_t { 
  return hash&((1<<global_depth_)-1);
}

auto ExtendibleHTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) const -> page_id_t { 
  return bucket_page_ids_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
  // 如果这个bucket_idx是新元素，设置local_depth
  bucket_page_ids_[bucket_idx]=bucket_page_id;
  if(global_depth_==0){
    current_size_++;
  }

}

// 这个函数要怎么实现呢？
// 获取分裂的新bucket的编号
// 例如初始桶是 B0000、B0001 depth=1
// B0001的桶满了之后
// 基于B0001分裂得到的新桶编号就是B0011 depth=2
// B0011就是对B0001的第(local_depth)位取反得到的
auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t {
  auto local_depth=GetLocalDepth(bucket_idx);
  auto local_mask=GetLocalDepthMask(bucket_idx);
  return (bucket_idx&local_mask)^(1<<(local_depth-1));
}

auto ExtendibleHTableDirectoryPage::GetGlobalDepth() const -> uint32_t { 
  return global_depth_;
}


void ExtendibleHTableDirectoryPage::IncrGlobalDepth() {
  auto old_depth=GetGlobalDepth();  // 保留旧的glo_D
  global_depth_++;
  assert(global_depth_<=max_depth_);
  uint32_t max_index=(1<<global_depth_)-1;  // bucket的数量翻倍
  uint32_t bucket_idx=(1<<old_depth);

  // 把新的下标分配到对应的bucket中
  // 新增bucket的Local_Depth继承旧的
  for(;bucket_idx<=max_index;bucket_idx++){
    auto old_mask=GetGlobalDepthMask()>>1;  // 比较低(Global_Depth-1)位
    local_depths_[bucket_idx]=local_depths_[bucket_idx&old_mask];
    bucket_page_ids_[bucket_idx]=bucket_page_ids_[bucket_idx&old_mask];
  }
  current_size_=max_index+1;
  std::cout<<"进行Global_Depth++后, Directory如下: "<<std::endl;
  PrintDirectory();
}

void ExtendibleHTableDirectoryPage::DecrGlobalDepth() {
  auto old_max_index=(1<<global_depth_)-1;
  global_depth_--;
  int index=(1<<global_depth_);
  for(;index<=old_max_index;index++){
    local_depths_[index]=0;
    bucket_page_ids_[index]=INVALID_PAGE_ID;
  }
  current_size_=1<<global_depth_;
  std::cout<<"进行Global_Depth--后, Directory如下: "<<std::endl;
  PrintDirectory();
}

// 如果所有的loc_D都小于glo_D，说明可以shrink
// 因为有一半bucket是没有被使用的
auto ExtendibleHTableDirectoryPage::CanShrink() -> bool { 
//   for(auto &elem:local_depths_){
//     if(elem==global_depth_){
//       return false;
//     }
//   }
//   return true;
return static_cast<bool>(std::all_of(local_depths_, local_depths_+HTABLE_DIRECTORY_ARRAY_SIZE, [this](auto elem) { return elem != global_depth_; }));
}

auto ExtendibleHTableDirectoryPage::Size() const -> uint32_t { 
  return current_size_;
}

auto ExtendibleHTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) const -> uint32_t { 
  return local_depths_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  local_depths_[bucket_idx]=local_depth;
}

void ExtendibleHTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) {
  // 如果local_detph不能直接++, 需要先进行global_depth++
  if(local_depths_[bucket_idx]==global_depth_){
    IncrGlobalDepth();
  }
  local_depths_[bucket_idx]++;
}

void ExtendibleHTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) {
  local_depths_[bucket_idx]--;
}

}  // namespace bustub
