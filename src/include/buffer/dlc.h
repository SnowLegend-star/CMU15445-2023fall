#pragma once

#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "common/config.h"
#include "storage/disk/disk_manager.h"
#include "storage/page/page.h"

#include <deque>
#include <optional>
#include <thread>
#include <unordered_map>
#include "ThreadPool.h"

namespace bustub {

// === DiskWriteRequest ===

/**
 * @brief 磁盘写请求结构体
 * 每个写请求对应一个页面ID及待写入的数据副本，包含同步原语以支持等待和通知机制。
 */
struct DiskWriteRequest {
  page_id_t page_id_;                            ///< 目标页面的ID
  char data_[BUSTUB_PAGE_SIZE];                  ///< 要写入磁盘的数据
  std::shared_ptr<std::condition_variable> cv_;  ///< 条件变量，用于线程等待/通知
  std::shared_ptr<std::mutex> mutex_;            ///< 请求对应的互斥锁
  bool is_cache_ = false;                        ///< 标记是否为缓存数据（最新写入完成的数据副本）

  DiskWriteRequest(page_id_t pid, const char *src)
      : page_id_(pid), cv_(std::make_shared<std::condition_variable>()), mutex_(std::make_shared<std::mutex>()) {
    memcpy(data_, src, BUSTUB_PAGE_SIZE);
  }
};

//*********************************************************************************************** */
// === DiskWriteRequestQueue ===
/**
 * @brief 管理针对同一页面的多个磁盘写请求的队列
 * 提供队列管理、同步等待以及缓存数据功能。
 */
class DiskWriteRequestQueue {
 public:
  // 向队列中加入新的写请求，并在队列原为空时通知处理线程
  void Push(const std::shared_ptr<DiskWriteRequest> &req) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool was_empty = queue_.empty();
    if (cache_ && cache_->is_cache_) {
      cache_ = nullptr;
    }
    queue_.push_back(req);
    if (was_empty) {
      queue_.front()->cv_->notify_one();
    }
    // std::cout << "向WRQ加入一个请求" << std::endl;
  }

  // 等待该请求成为队首，以确保磁盘写入顺序
  auto WaitUntilHead(std::shared_ptr<DiskWriteRequest> req, const std::atomic<bool> &stop_flag)
      -> std::shared_ptr<DiskWriteRequest> {
    std::unique_lock<std::mutex> lock(mutex_);
    req->cv_->wait(lock, [&] {
      // 如果 stop_flag 是 true，强制退出等待
      // req->cv_->wait(lock, [&] { return queue_.front() == req || stop_flag.load(); });
      // 上述内容是导致线程无法征程退出的根源！！
      if (stop_flag) {
        return true;
      }
      return !queue_.empty() && queue_.front() == req;
    });
    if (stop_flag) {
      return nullptr;
    }
    return req;
  }

  // 请求完成后弹出队首并通知下一个请求（若存在）
  void PopFrontAndNotify() {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.pop_front();
    if (!queue_.empty()) {
      queue_.front()->cv_->notify_one();
    }
  }

  // 尝试获取最新的数据副本（队尾或缓存数据）
  auto TryGetLatestData() -> std::optional<std::shared_ptr<DiskWriteRequest>> {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!queue_.empty()) {
      return queue_.back();
    }
    if (cache_) {
      return cache_;
    }
    return std::nullopt;
  }

  // 设置缓存（最新写入完成的数据副本）
  void SetCache(const std::shared_ptr<DiskWriteRequest> &req) {
    std::unique_lock<std::mutex> lock(mutex_);
    req->is_cache_ = true;
    cache_ = req;
  }

  // 判断队列是否为空
  auto Empty() -> bool {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  // 安全地通知队列内所有等待线程退出
  void NotifyAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &req : queue_) {
      req->cv_->notify_all();
    }
  }

 private:
  std::deque<std::shared_ptr<DiskWriteRequest>> queue_;  ///< 存放待写入的请求
  std::shared_ptr<DiskWriteRequest> cache_;              ///< 最新完成写入的请求（缓存）
  std::mutex mutex_;                                     ///< 队列的互斥锁
};

// //*********************************************************************************************** */
// /**
//  * @brief 磁盘管理器代理类，负责写入磁盘和读取数据的协调工作
//  * 使用后台线程统一处理磁盘写入任务，以优化线程资源使用。
//  */
// class DiskManagerProxy {
//  public:
//   explicit DiskManagerProxy(DiskManager *disk_manager) : disk_manager_(disk_manager), stop_(false) {
//     worker_thread_ = std::thread(&DiskManagerProxy::ProcessWriteRequests, this);
//   }

//   ~DiskManagerProxy() {
//     stop_ = true;
//     cv_.notify_all();  // 唤醒等待 map_mutex_ 的线程

//     {
//       std::lock_guard<std::mutex> lock(map_mutex_);
//       for (auto &[_, queue] : queues_) {
//         queue.NotifyAll();  // 唤醒所有在每个 queue 等待的请求
//       }
//     }

//     if (worker_thread_.joinable()) {
//       worker_thread_.join();  // 如果后台线程还在运行，一定要等待它结束
//     }
//   }

//   // 向指定页面写入数据
//   void WritePage(page_id_t page_id, const char *data) {
//     auto request = std::make_shared<DiskWriteRequest>(page_id, data);
//     {
//       std::unique_lock<std::mutex> lock(map_mutex_);
//       auto &queue = queues_[page_id];
//       queue.Push(request);
//     }
//     cv_.notify_one();
//   }

//   // 读取指定页面数据，优先返回队列中或缓存中的最新副本
//   auto ReadPage(page_id_t page_id, char *out_data) -> bool {
//     std::unique_lock<std::mutex> lock(map_mutex_);
//     auto &queue = queues_[page_id];
//     auto latest = queue.TryGetLatestData();

//     if (latest) {
//       std::memcpy(out_data, (*latest)->data_, BUSTUB_PAGE_SIZE);
//       return true;
//     }

//     // 若无最新副本，则直接从磁盘读取
//     disk_manager_->ReadPage(page_id, out_data);
//     return true;
//   }

//  private:
//   // 后台线程处理所有磁盘写入请求
//   void ProcessWriteRequests() {
//     while (!stop_) {
//       std::unique_lock<std::mutex> lock(map_mutex_);
//       cv_.wait(lock, [this] {
//         return stop_ || std::any_of(queues_.begin(), queues_.end(), [](auto &pair) { return !pair.second.Empty(); });
//       });

//       if (stop_) {
//         break;
//       }

//       for (auto &[page_id, queue] : queues_) {
//         while (!queue.Empty()) {
//           auto req_opt = queue.TryGetLatestData();
//           if (!req_opt) {
//             break;
//           }

//           // 释放 map 锁，避免锁住其他线程访问 queues_
//           lock.unlock();

//           auto req = queue.WaitUntilHead(*req_opt, stop_);
//           if (!req) {
//             return;  // 提前退出
//           }

//           disk_manager_->WritePage(req->page_id_, req->data_);
//           queue.PopFrontAndNotify();
//           if (queue.Empty()) {
//             queue.SetCache(req);
//           }

//           lock.lock();  // 重新加锁 map_mutex_
//         }
//       }
//     }
//   }

//   std::unordered_map<page_id_t, DiskWriteRequestQueue> queues_;  ///< 页面ID到请求队列的映射
//   std::mutex map_mutex_;                                         ///< 映射表的互斥锁
//   std::condition_variable cv_;                                   ///< 用于通知后台线程
//   DiskManager *disk_manager_;                                    ///< 实际磁盘管理器
//   std::thread worker_thread_;                                    ///< 后台处理线程
//   std::atomic<bool> stop_;                                       ///< 停止线程标志
// };

class DiskManagerProxy {
 public:
  explicit DiskManagerProxy(DiskManager *disk_manager, size_t n_workers = 4)
      : disk_manager_(disk_manager), stop_(false), pool_(n_workers) {}

  ~DiskManagerProxy() { Shutdown(); }

  /* ============== 对外 API ============== */

  /** 异步写：立即返回，不阻塞调用线程 */
  void WritePage(page_id_t pid, const char *data) {
    auto req = std::make_shared<DiskWriteRequest>(pid, data);
    bool push_ready = false;
    {
      std::lock_guard<std::mutex> m(map_mtx_);
      auto &q = queues_[pid];
      push_ready = q.Empty();
      q.Push(req);
    }
    if (push_ready) {
      EnqueueReady(pid);
    }
  }

  /** 读取：若内存里有最新副本直接 memcpy，否则同步磁盘读 */
  auto ReadPage(page_id_t pid, char *out_data) -> bool {
    {
      std::lock_guard<std::mutex> m(map_mtx_);
      auto &q = queues_[pid];
      if (auto latest = q.TryGetLatestData(); latest) {
        memcpy(out_data, (*latest)->data_, BUSTUB_PAGE_SIZE);
        return true;
      }
    }
    /* miss -> 同步磁盘 */
    disk_manager_->ReadPage(pid, out_data);
    return true;
  }

  /* ============== 停机 ============== */
  void Shutdown() {
    if (stop_.exchange(true)) {
      return;  // 已关
    }
    ready_cv_.notify_all();  // 唤醒 pool 内任何等待取任务的线程
    /* 线程池析构自动等待所有任务完成 */
  }

 private:
  /* ---------------- 内部调度 ---------------- */

  /** 把新的就绪 page_id 推入 ready 队列并提交到线程池 */
  void EnqueueReady(page_id_t pid) {
    {
      std::lock_guard<std::mutex> lk(ready_mtx_);
      ready_pages_.push_back(pid);
    }
    ready_cv_.notify_one();

    /* 只需要一次提交 WorkerLoop，多个线程并发调用也安全 */
    pool_.Submit([this] { WorkerLoop(); });
  }

  /** 多 worker 共享的循环：一次写一条请求，必要时把 page_id 再丢回 ready 队列 */
  void WorkerLoop() {
    while (true) {
      /* 1. 取一个就绪 page_id */
      page_id_t pid;
      {
        std::unique_lock<std::mutex> lk(ready_mtx_);
        ready_cv_.wait(lk, [&] { return stop_ || !ready_pages_.empty(); });
        if (stop_) {
          return;
        }
        pid = ready_pages_.front();
        ready_pages_.pop_front();
      }

      /* 2. 拿该页的最新请求（队尾） */
      std::shared_ptr<DiskWriteRequest> req;
      {
        std::lock_guard<std::mutex> m(map_mtx_);
        auto &q = queues_[pid];
        auto opt = q.TryGetLatestData();
        if (!opt) {
          continue;  // 本页被其他线程清空，处理下一页
        }
        req = *opt;
      }

      /* 3. 等到自己成为队首（串行保证） */
      req = queues_[pid].WaitUntilHead(req, stop_);
      if (!req) {
        return;  // 正在停机
      }

      /* 4. 实际写盘 */
      disk_manager_->WritePage(pid, req->data_);

      /* 5. 出队 + 决定是否重新入 ready */
      bool requeue = false;
      {
        std::lock_guard<std::mutex> m(map_mtx_);
        auto &q = queues_[pid];
        q.PopFrontAndNotify();
        if (q.Empty()) {
          q.SetCache(req);  // 更新 cache
        } else {
          requeue = true;  // 仍有剩余请求
        }
      }
      if (requeue) {
        EnqueueReady(pid);
      }
    }
  }

  /* ---------------- 成员变量 ---------------- */
  DiskManager *disk_manager_;  // 实际磁盘管理器
  /* 每页私有请求队列 */
  std::unordered_map<page_id_t, DiskWriteRequestQueue> queues_;  // 页面ID到请求队列的映射
  std::mutex map_mtx_;      // 映射表的互斥锁

  std::deque<page_id_t> ready_pages_;  // 全局就绪页队列 
  std::mutex ready_mtx_;  
  std::condition_variable ready_cv_;
  std::atomic<bool> stop_;  //停止线程标志
  ThreadPool pool_;  // 固定大小线程池
};

}  // namespace bustub