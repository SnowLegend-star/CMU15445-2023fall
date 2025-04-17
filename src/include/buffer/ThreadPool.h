#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @brief 极简固定大小线程池，支持提交任意可调用对象。
 * 不返回 future（写出问题了直接报错）
 */
class ThreadPool {
 public:
  explicit ThreadPool(size_t n_threads = std::thread::hardware_concurrency()) : stop_(false) {
    workers_.reserve(n_threads);
    for (size_t i = 0; i < n_threads; ++i) {
      workers_.emplace_back([this] { WorkerLoop(); });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lk(queue_mtx_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto &t : workers_) {
      if (t.joinable()) {
        t.join();
      }
    }
  }

  template <typename Fn>
  void Submit(Fn &&task) {
    {
      std::lock_guard<std::mutex> lk(queue_mtx_);
      tasks_.emplace(std::forward<Fn>(task));
    }
    cv_.notify_one();
  }

 private:
  void WorkerLoop() {
    while (true) {
      std::function<void()> job;
      {
        std::unique_lock<std::mutex> lk(queue_mtx_);
        cv_.wait(lk, [&] { return stop_ || !tasks_.empty(); });
        if (stop_ && tasks_.empty()) {
          return;
        }
        job = std::move(tasks_.front());
        tasks_.pop();
      }
      job();
    }
  }

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mtx_;
  std::condition_variable cv_;
  std::atomic<bool> stop_;
};
