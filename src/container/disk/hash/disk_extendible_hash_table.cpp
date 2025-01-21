//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.cpp
//
// Identification: src/container/disk/hash/disk_extendible_hash_table.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/macros.h"
#include "common/rid.h"
#include "common/util/hash_util.h"
#include "container/disk/hash/disk_extendible_hash_table.h"
#include "storage/index/hash_comparator.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

template <typename K, typename V, typename KC>
DiskExtendibleHashTable<K, V, KC>::DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm,
                                                           const KC &cmp, const HashFunction<K> &hash_fn,
                                                           uint32_t header_max_depth, uint32_t directory_max_depth,
                                                           uint32_t bucket_max_size)
    : bpm_(bpm),
      cmp_(cmp),
      hash_fn_(std::move(hash_fn)),
      header_max_depth_(header_max_depth),
      directory_max_depth_(directory_max_depth),
      bucket_max_size_(bucket_max_size) {
  // throw NotImplementedException("DiskExtendibleHashTable is not implemented");
  index_name_=name;
  BasicPageGuard header_guard=bpm->NewPageGuarded( &header_page_id_);
  auto header_page=header_guard.AsMut<ExtendibleHTableHeaderPage>();
  header_page->Init(header_max_depth);
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::GetValue(const K &key, std::vector<V> *result, Transaction *transaction) const
    -> bool {
  // 分为两步走
  // 1、得到对应的diectory进行遍历
  // 2、找到相应的bucket进行遍历

  auto hash=Hash(key);
  ReadPageGuard header_read_guard=bpm_->FetchPageRead(header_page_id_);
  auto header_page=header_read_guard.As<ExtendibleHTableHeaderPage>();
  auto directroy_index=header_page->HashToDirectoryIndex(hash);
  int directory_page_id=header_page->GetDirectoryPageId(directroy_index);
  if(directory_page_id==INVALID_PAGE_ID){
    return false;
  }

  // 在Header中成功找到对应dir的表项，遍历这个Directory继续寻找bucket
  ReadPageGuard dir_read_guard=bpm_->FetchPageRead(directory_page_id);
  auto dir_page=dir_read_guard.As<ExtendibleHTableDirectoryPage>();
  auto bucket_index=dir_page->HashToBucketIndex(hash);
  int bucket_page_id=dir_page->GetBucketPageId(bucket_index);
  if(bucket_page_id==INVALID_PAGE_ID){
    return false;
  }

  // 成功找到bucket的表项，开始遍历bucket
  V value;
  ReadPageGuard buk_read_page=bpm_->FetchPageRead(bucket_page_id);
  auto buk_page=buk_read_page.As<ExtendibleHTableBucketPage<K, V, KC>>();
  if(buk_page->Lookup(key,value,cmp_)){
    result->push_back(value);
    return true;
  }

  return false;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/


// 什么时候进行Glo_D的++操作
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Insert(const K &key, const V &value, Transaction *transaction) -> bool {
  auto hash=Hash(key);
  WritePageGuard header_guard=bpm_->FetchPageWrite(header_page_id_); // 涉及对Header页面的写入
  auto header_page=header_guard.AsMut<ExtendibleHTableHeaderPage>(); // 需不需要改动页面呢？如果这里用As, 就不能传递给InsertToNewDir了
  auto directory_index=header_page->HashToDirectoryIndex(hash);
  int directory_page_id=header_page->GetDirectoryPageId(directory_index);
  bool insert_success=false;
  // 如果这个目录页是空的
  if(directory_page_id==INVALID_PAGE_ID){
    insert_success=InsertToNewDirectory(header_page,  directory_index,  hash, key, value);
    return insert_success;
  }
  // 目录不空, 定位到对应的Directory
  WritePageGuard dir_write_guard=bpm_->FetchPageWrite(directory_page_id);
  auto dir_page=dir_write_guard.AsMut<ExtendibleHTableDirectoryPage>();
  auto buk_index=dir_page->HashToBucketIndex(hash);
  int buk_page_id=dir_page->GetBucketPageId(buk_index);

  // 如果这个bucket是空的
  if(buk_page_id==INVALID_PAGE_ID){
    insert_success=InsertToNewBucket(dir_page, buk_index, key, value);
    return insert_success;
  }
  // 这个bucket存在, 开始进行insert
  WritePageGuard buk_write_guard=bpm_->FetchPageWrite(buk_page_id);
  auto buk_page=buk_write_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  insert_success=false;
  V tmp_value;
  // 如果这个key已经存在了，直接返回false
  if(buk_page->Lookup(key,tmp_value,cmp_)){
    return false;
  }
  // bucket存在且没满
  if(!buk_page->IsFull()){
      insert_success=buk_page->Insert(key,value,cmp_);
      return insert_success;
  }
  // 如果bucket存在，但是已经满了就开始进行分裂并重映射
  // 向新的Directory重新插入这个key直至成功
  if(buk_page->IsFull()){
    if(dir_page->GetGlobalDepth()==dir_page->GetLocalDepth(buk_index)){ // 都满了已经不能继续扩张了
      if(dir_page->GetGlobalDepth()==dir_page->GetMaxDepth()){
        return false;
      }
    }
    dir_page->IncrLocalDepth(buk_index);
    // 获取一个新的bucket页
    page_id_t new_bucket_page_id;
    auto new_page_guard=bpm_->NewPageGuarded(&new_bucket_page_id);
    auto bucket_write_page=new_page_guard.UpgradeWrite();
    auto new_bucket_page=bucket_write_page.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
    new_bucket_page->Init(bucket_max_size_);

    // 准备将Directory内的bucket页表项进行重映射
    auto new_bucket_idx=dir_page->GetSplitImageIndex(buk_index);
    auto new_local_depth=dir_page->GetLocalDepth(buk_index);
    auto local_depth_mask=dir_page->GetLocalDepthMask(buk_index);
    UpdateDirectoryMapping(dir_page,  new_bucket_idx,  new_bucket_page_id,  new_local_depth,  local_depth_mask);
    uint32_t i=0;
    // 移动元素时有个要注意的点
    // 移动一个元素后，下标会动态变化的!
    for(;i<buk_page->Size();i++){
      auto tmp_key=buk_page->KeyAt(i);
      auto tmp_val=buk_page->ValueAt(i);
      auto new_hash=Hash(tmp_key);
      auto rehash_buk_idx=dir_page->HashToBucketIndex(new_hash);
      auto rehash_page_id=dir_page->GetBucketPageId(rehash_buk_idx);
      if(rehash_page_id==new_bucket_page_id){
        // 先移动后remove
        new_bucket_page->Insert(tmp_key,tmp_val,cmp_);
        buk_page->RemoveAt(i);
        i--;
      }
    }

    // 完成重映射后, 插入新的键值对
    buk_index=dir_page->HashToBucketIndex(hash);
    buk_page_id=dir_page->GetBucketPageId(buk_index);
    if(buk_page_id==new_bucket_page_id){
      insert_success=new_bucket_page->Insert(key,value,cmp_);
    }else{
      insert_success=buk_page->Insert(key,value,cmp_);
    }
  }

  buk_write_guard.Drop(); // 打印之前手动释放这个Bucket页面
  std::cout<<"插入元素key: "<<key<<"后, Directory和Bucket分别为:"<<std::endl;
  dir_page->PrintDirectory();
  uint32_t i=0;
  for(;i<dir_page->Size();i++){
    auto page_id=dir_page->GetBucketPageId(i);
    auto read_guard=bpm_->FetchPageRead(page_id);
    auto read_page=read_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();
    read_page->MyPrintBucket(page_id);
  }
  return insert_success;
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewDirectory(ExtendibleHTableHeaderPage *header, uint32_t directory_idx,
                                                             uint32_t hash, const K &key, const V &value) -> bool {
  // 在Header页中设置新的Directory页表项
  page_id_t directory_page_id;
  BasicPageGuard directory_guard=bpm_->NewPageGuarded(&directory_page_id);
  auto directory_write_guard=directory_guard.UpgradeWrite();  // 需要更改这个页面
  auto directory_page=directory_write_guard.AsMut<ExtendibleHTableDirectoryPage>();
  directory_page->Init(directory_max_depth_);
  header->SetDirectoryPageId(directory_idx, directory_page_id);

  auto bucket_idx=directory_page->HashToBucketIndex(hash);
  // 级联插入成功才是真正的成功
  return InsertToNewBucket(directory_page,  bucket_idx, key, value);
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewBucket(ExtendibleHTableDirectoryPage *directory, uint32_t bucket_idx,
                                                          const K &key, const V &value) -> bool {

  page_id_t bucket_page_id;
  auto tmp_bucket_guard = bpm_->NewPageGuarded(&bucket_page_id);
  auto bucket_guard = tmp_bucket_guard.UpgradeWrite();
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  bucket_page->Init(bucket_max_size_);

  // 这里如果bucket满了则如何呢？？这里bucket才刚创建，还不会满
  auto insert_success=bucket_page->Insert(key,value,cmp_);
  // 在新的Directory页中设置新的Bucket页表项
  directory->SetBucketPageId( bucket_idx,  bucket_page_id);
  directory->SetLocalDepth(bucket_idx,  0);
  return insert_success;
}

// 准备将Directory内的bucket页表项进行重映射
template <typename K, typename V, typename KC>
void DiskExtendibleHashTable<K, V, KC>::UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory,
                                                               uint32_t new_bucket_idx, page_id_t new_bucket_page_id,
                                                               uint32_t new_local_depth, uint32_t local_depth_mask) {
  uint32_t i=0;
  // 把Directory中所有低(new_local_depth)位相同的映射到新的new_bucket_page_id
  for(;i<directory->Size();i++){
    if((i&local_depth_mask)==(new_bucket_idx&local_depth_mask)){
      directory->SetBucketPageId(i,  new_bucket_page_id);      
    }
  }
  directory->SetBucketPageId(new_bucket_idx,  new_bucket_page_id);
  directory->SetLocalDepth(new_bucket_idx, new_local_depth);
  std::cout<<"UpdateDirectoryMapping得到的结果是: "<<std::endl;
  directory->PrintDirectory();
}

/*****************************************************************************
 * REMOVE
  涉及Bucket合并
  如果B00和B10都空了, 则它俩应该合并  ->  是不是有一个page要被删掉了呢?
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Remove(const K &key, Transaction *transaction) -> bool {
  WritePageGuard header_guard=bpm_->FetchPageWrite(header_page_id_);
  auto header_page=header_guard.AsMut<ExtendibleHTableHeaderPage>();
  auto hash=Hash(key);
  auto dir_index=header_page->HashToDirectoryIndex(hash);
  int dir_page_id=header_page->GetDirectoryPageId(dir_index);
  if(dir_page_id==INVALID_PAGE_ID){ // Directory不存在
    return false;
  }
  // 找到了相应的Directory
  WritePageGuard dir_guard=bpm_->FetchPageWrite(dir_page_id);
  auto dir_page=dir_guard.AsMut<ExtendibleHTableDirectoryPage>();
  auto bucket_index=dir_page->HashToBucketIndex(hash);
  int bucket_page_id=dir_page->GetBucketPageId(bucket_index);
  if(bucket_page_id==INVALID_PAGE_ID){ // Bucket不存在
    return false;
  }
  // 找到了相应的Bucket
  WritePageGuard bucket_guard=bpm_->FetchPageWrite(bucket_page_id);
  auto bucket_page=bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  auto remove_res=bucket_page->Remove(key,cmp_);

  // remove查看是否可以合并  有三种情况
  if(remove_res){ 
    // 1、local_depth为0, 说明它并没有bro
    auto local_depth=dir_page->GetLocalDepth(bucket_index);
    if(local_depth==0){
      return remove_res;
    }

    // 2、有bro, 但是bro和它指向相同的page
    auto bro_page_index=dir_page->GetSplitImageIndex(bucket_index);
    auto bro_page_id=dir_page->GetBucketPageId(bro_page_index);
    if(bro_page_id==bucket_page_id){
      return remove_res;
    }

    // 3、有bro, 且bro和它指向不同的page   会不会存在并发问题呢
    WritePageGuard bro_page_guard=bpm_->FetchPageWrite(bro_page_id);
    auto bro_page=bro_page_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
    while(bucket_page->IsEmpty()&&bro_page->IsEmpty()){
      dir_page->DecrLocalDepth(bucket_index);
      dir_page->DecrLocalDepth(bro_page_index); // bro页面的Local Depth也要调整, 因为在Directory中page_id_和local_depth_是分别存储的
      bro_page_guard.Drop();
      bpm_->DeletePage(bro_page_id);
      auto new_local_depth=dir_page->GetLocalDepth(bucket_index);
      auto new_loacl_mask=dir_page->GetLocalDepthMask(bucket_index);
      UpdateDirectoryMapping(dir_page,bucket_index,bucket_page_id,new_local_depth,new_loacl_mask);
      if(dir_page->CanShrink()){
        dir_page->DecrGlobalDepth();
      }
      // 更新bro看看有没有级联删除的可能
      if(new_local_depth==0){
        break;
      }
      bro_page_index=dir_page->GetSplitImageIndex(bucket_index);
      bro_page_id=dir_page->GetBucketPageId(bro_page_index);
      if(bro_page_id==bucket_page_id){
        break;
      }
    }
  }

  bucket_guard.Drop();  // 提前释放, 防止一会儿打印的时候死锁
  std::cout<<"Remove key: "<<key<<"后, Directory和Bucket如下: "<<std::endl;
  dir_page->PrintDirectory();
  uint32_t i=0;
  for(;i<dir_page->Size();i++){
    auto page_id=dir_page->GetBucketPageId(i);
    auto read_guard=bpm_->FetchPageRead(page_id);
    auto read_page=read_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();
    read_page->MyPrintBucket(page_id);
  }
  return remove_res;
}

template class DiskExtendibleHashTable<int, int, IntComparator>;
template class DiskExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class DiskExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class DiskExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class DiskExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class DiskExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
