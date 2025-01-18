//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.cpp
//
// Identification: src/storage/page/extendible_htable_header_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_header_page.h"
#include <cstdint>

#include "common/config.h"
#include "common/exception.h"

namespace bustub {

void ExtendibleHTableHeaderPage::Init(uint32_t max_depth) {
  // throw NotImplementedException("ExtendibleHTableHeaderPage is not implemented");
  max_depth_=max_depth;

  for(int & directory_page_id : directory_page_ids_){
    directory_page_id=INVALID_PAGE_ID;
  }
}

// 有点不理解这个函数的意思
// 就是取hash的高max_depth位，一共有2^max_depth个目录
auto ExtendibleHTableHeaderPage::HashToDirectoryIndex(uint32_t hash) const -> uint32_t { 
  if(max_depth_==0){
    return 0;
  }
  return hash>>(32-max_depth_)&(HTABLE_HEADER_ARRAY_SIZE-1);
}

auto ExtendibleHTableHeaderPage::GetDirectoryPageId(uint32_t directory_idx) const -> uint32_t { 
  return directory_page_ids_[directory_idx];
}

void ExtendibleHTableHeaderPage::SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id) {
  directory_page_ids_[directory_idx]=directory_page_id;
}

auto ExtendibleHTableHeaderPage::MaxSize() const -> uint32_t { 
  return (1<<max_depth_);
}

}  // namespace bustub
