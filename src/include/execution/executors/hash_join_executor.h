//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.h
//
// Identification: src/include/execution/executors/hash_join_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/hash_join_plan.h"
#include "storage/table/tuple.h"
#include "type/value.h"

namespace bustub {
/** HashJoinKey 表示哈希操作中的一个键 */
struct HashJoinKey {
  std::vector<Value> hash_keys_;

  /**
    *当我们需要将 HashJoinKey 对象作为哈希表（比如 unordered_map 或 unordered_set）中的键时，
    哈希表会使用 operator== 来比较不同的键是否相等
  */
  auto operator==(const HashJoinKey &other) const -> bool {
    for (uint32_t i = 0; i < other.hash_keys_.size(); i++) {
      if (hash_keys_[i].CompareEquals(other.hash_keys_[i]) != CmpBool::CmpTrue) {
        return false;
      }
    }
    return true;
  }
};

/** 每个hashkey都有对应的hashvalue */
struct HashJoinValue {
  /** 哈希值集合 */
  std::vector<Tuple> hashjoin_;
};
}  // namespace bustub

namespace std {

/** 当我们将 HashJoinKey 放入一个 unordered_map 或 unordered_set 中时，
 *C++ 会自动调用 std::hash<bustub::HashJoinKey> 的 operator() 来计算该对象的哈希值。
 **/
template <>
struct hash<bustub::HashJoinKey> {
  auto operator()(const bustub::HashJoinKey &hash_key) const -> std::size_t {
    size_t curr_hash = 0;
    for (const auto &key : hash_key.hash_keys_) {
      if (!key.IsNull()) {
        curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&key));
      }
    }
    return curr_hash;
  }
};

}  // namespace std

namespace bustub {

/**
 * 一个简化的哈希表，存储left table对应hashjoin的哈希值
 */
class SimpleHashJoinHashTable {
 public:
  /**
   * 将一个值插入到哈希表中
   * @param hash_key 要插入的键
   * @param hash_val 要插入的值
   */
  void InsertKey(const HashJoinKey &hash_key, const Tuple &tuple) {
    auto tmp_value = GetValue(hash_key);
    if (tmp_value.hashjoin_.empty()) {
      std::vector<Tuple> tuples;
      tuples.emplace_back(tuple);
      ht_.insert({hash_key, {tuples}});
      return;
    }
    tmp_value.hashjoin_.push_back(tuple);
    ht_[hash_key] = HashJoinValue{tmp_value};
  }

  /** 获得某个hash_key对应的value**/
  auto GetValue(const HashJoinKey &hash_key) -> HashJoinValue {
    if (ht_.find(hash_key) == ht_.end()) {
      return {};
    }
    return ht_.find(hash_key)->second;
  }

  /**
   * Clear the hash table
   */
  void Clear() { ht_.clear(); }

  /** 聚合哈希表的迭代器 */
  class Iterator {
   public:
    /** 为聚合映射创建一个迭代器。 */
    explicit Iterator(std::unordered_map<HashJoinKey, HashJoinValue>::const_iterator iter) : iter_{iter} {}

    /** @return 迭代器的键 */
    auto Key() -> const HashJoinKey & { return iter_->first; }

    /** @return 迭代器的值 */
    auto Val() -> const HashJoinValue & { return iter_->second; }

    /** @return 迭代器递增前的迭代器 */
    auto operator++() -> Iterator & {
      ++iter_;
      return *this;
    }

    /** @return `true` 如果两个迭代器相等 */
    auto operator==(const Iterator &other) -> bool { return this->iter_ == other.iter_; }

    /** @return `true` 如果两个迭代器不相等 */
    auto operator!=(const Iterator &other) -> bool { return this->iter_ != other.iter_; }

   private:
    /** 聚合映射 */
    std::unordered_map<HashJoinKey, HashJoinValue>::const_iterator iter_;
  };

  /** @return 哈希表的开始迭代器 */
  auto Begin() -> Iterator { return Iterator{ht_.cbegin()}; }

  /** @return 哈希表的结束迭代器 */
  auto End() -> Iterator { return Iterator{ht_.cend()}; }

 private:
  //这个哈希表做聚合键到聚合值的映射
  std::unordered_map<HashJoinKey, HashJoinValue> ht_{};
};

/**
 * HashJoinExecutor 执行两个表的哈希连接。
 */
class HashJoinExecutor : public AbstractExecutor {
 public:
  /**
   * 构造一个新的 HashJoinExecutor 实例。
   * @param exec_ctx 执行器上下文
   * @param plan 要执行的 HashJoin 连接计划
   * @param left_child 提供左侧连接元组的子执行器
   * @param right_child 提供右侧连接元组的子执行器
   */
  HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                   std::unique_ptr<AbstractExecutor> &&left_child, std::unique_ptr<AbstractExecutor> &&right_child);

  /** 初始化连接操作 */
  void Init() override;

  /**
   * 从连接操作中获取下一个元组。
   * @param[out] tuple 连接操作产生的下一个元组
   * @param[out] rid 下一个元组的 RID，哈希连接不使用该值
   * @return 如果生成了元组，返回 `true`；如果没有更多元组，返回 `false`
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return 连接操作的输出模式 */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  /*获得left table某个元组对应的hash_key*/
  auto MakeLeftTupleHashKey(const Tuple *tuple) -> HashJoinKey {
    std::vector<Value> keys;
    for (auto &expr : plan_->LeftJoinKeyExpressions()) {
      keys.emplace_back(expr->Evaluate(tuple, left_executor_->GetOutputSchema()));
    }
    return {keys};
  }

  /*获得right table某个元组对应的hash_key*/
  auto MakeRightTupleHashKey(const Tuple *tuple) -> HashJoinKey {
    std::vector<Value> keys;
    for (auto &expr : plan_->RightJoinKeyExpressions()) {
      keys.emplace_back(expr->Evaluate(tuple, right_executor_->GetOutputSchema()));
    }
    return {keys};
  }

  /** 要执行的 HashJoin 计划节点 */
  const HashJoinPlanNode *plan_;
  std::unique_ptr<AbstractExecutor> left_executor_;
  std::unique_ptr<AbstractExecutor> right_executor_;
  SimpleHashJoinHashTable hashtable_{};  // 用来存储left table的哈希表

  Tuple left_cur_tuple_;  // 记录left table正在处理的元组
  bool left_status_;      // 此次获取left table的元素是否成功
  bool tuple_match_;      // 这次left tuple是否找到了对应的right tuple  left tuple必然会找到对应的
  std::vector<Tuple> correspond_value_;  // 对应hash key中的tuple集合
  std::vector<Tuple>::iterator iter_;    // 这个tuple集合的迭代器
};

}  // namespace bustub
