#pragma once

#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {

// 访问类型枚举，用于表示对页的访问类型
enum class AccessType { Unknown = 0, Lookup, Scan, Index };

// LRUK节点类
class LRUKNode {
 private:
  /** 页面的最近K次访问时间戳历史记录。最早的时间戳存储在最前面。 */
  // 如果开始使用，请删除maybe_unused。随意更改成员变量。

 public:
  std::list<size_t> history_;  // 存储最近的K个时间戳历史记录
  size_t k_;                   // K值，表示最近访问的历史记录数量
  frame_id_t fid_;             // frameID，表示页面ID
  bool is_evictable_{false};   // 是否可淘汰

  auto GetBackKTimeStamp() { return history_.front(); }   // 获取最早的K个时间戳
  auto GetLatestTimeStamp() { return history_.front(); }  // 获取最新的时间戳
};

// LRUK替换器类，实现LRU-k替换策略
class LRUKReplacer {
 public:
  /**
   *
   * TODO(P1): 添加实现
   *
   * @brief 创建一个新的LRUK替换器。
   * @param num_frames LRUReplacer需要存储的最大frame数
   */
  explicit LRUKReplacer(size_t num_frames, size_t k);

  DISALLOW_COPY_AND_MOVE(LRUKReplacer);  // 禁止复制和移动

  /**
   * TODO(P1): 添加实现
   *
   * @brief 销毁LRUReplacer。
   */
  ~LRUKReplacer() = default;

  /**
   * TODO(P1): 添加实现
   *
   * @brief 查找具有最大后向K距离的frame，并驱逐该frame。只有被标记为“可淘汰”的frame才是淘汰的候选对象。
   *
   * 具有少于K个历史引用的frame的后向K距离被赋予+inf。如果多个frame具有+inf的后向K距离，则根据LRU选择最早时间戳的frame作为受害者。
   *
   * 成功驱逐frame应该减少替换器的大小并删除frame的访问历史。
   *
   * @param[out] frame_id 被驱逐的frame的ID。
   * @return 如果成功驱逐了一个frame，则返回true；如果没有frame可以被驱逐，则返回false。
   */
  auto Evict(frame_id_t *frame_id) -> bool;  //找出可以替换的物理页ID并赋值给参数，若没有可以替换的则返回false

  /**
   * TODO(P1): 添加实现
   *
   * @brief 记录给定frameID在当前时间戳的访问事件。
   * 如果frameID无效（即大于replacer_size_），则抛出异常。如果frameID无效，也可以使用BUSTUB_ASSERT终止进程。
   *
   * @param frame_id 接收新访问的frame的ID。
   * @param access_type 接收的访问类型。此参数仅在排行榜测试中需要。
   */
  void RecordAccess(
      frame_id_t frame_id,
      AccessType access_type =
          AccessType::Unknown);  //记录给定物理页访问的时间戳，若不存在与物理页映射的虚拟页那么创建然后再加入

  /**
   * TODO(P1): 添加实现
   *
   * @brief 切换frame是否可淘汰。此函数还控制替换器的大小。注意，大小等于可淘汰条目的数量。
   *
   * 如果frame之前是可淘汰的，并且要设置为不可淘汰，则大小应减少。如果frame之前不可淘汰，并且要设置为可淘汰，则大小应增加。
   *
   * 如果frameID无效，则抛出异常或终止进程。
   *
   * 对于其他情况，此函数应该在不修改任何内容的情况下终止。
   *
   * @param frame_id 要修改其“可淘汰”状态的frame的ID
   * @param set_evictable 指定的frame是否可淘汰
   */
  void SetEvictable(frame_id_t frame_id, bool set_evictable);  //设置物理页ID 的可替换为 set_evictable

  /**
   * TODO(P1): 添加实现
   *
   * @brief 从替换器中移除一个可淘汰frame，以及其访问历史记录。
   * 如果移除成功，此函数还应减少替换器的大小。
   *
   * 请注意，这与驱逐frame不同，后者始终删除具有最大后向K距离的frame。此函数删除指定的frameID，无论其后向K距离如何。
   *
   * 如果在不可淘汰的frame上调用Remove，则抛出异常或终止进程。
   *
   * 如果未找到指定的frame，则直接从此函数返回。
   *
   * @param frame_id 要移除的frame的ID
   */
  void Remove(frame_id_t frame_id);

  /**
   * TODO(P1): 添加实现
   *
   * @brief 返回替换器的大小，即跟踪可淘汰frame的数量。
   *
   * @return size_t
   */
  auto Size() -> size_t;

 private:
  // 学生应该实现这些成员变量！如果开始使用，请删除maybe_unused。
  std::unordered_map<frame_id_t, LRUKNode> node_store_;  // 存储LRUK节点的哈希表
  size_t current_timestamp_{0};                          // 当前时间戳
  size_t current_size_{0};                               // 当前大小
  size_t replacer_size_;                                 // 替换器大小
  size_t k_;                                             // k值
  std::mutex latch_;                                     // 互斥量，用于保护并发访问
};

}  // namespace bustub
