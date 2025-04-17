#include <unordered_map>
#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include "storage/disk/disk_manager.h"


class ThreadPool {
    public:
     explicit ThreadPool(size_t size) : stop_(false) {
       for (size_t i = 0; i < size; ++i) {
         workers_.emplace_back(&ThreadPool::WorkerLoop, this);  // 使用成员函数
       }
     }
   
     ~ThreadPool() {
       {
         std::lock_guard<std::mutex> lock(mutex_);
         stop_ = true;
       }
       cond_.notify_all();  // 唤醒所有工作线程
       for (auto &worker : workers_) {
         if (worker.joinable()) {
           worker.join();  // 等待所有线程退出
         }
       }
     }
     // 把外部提交的任务（可调用对象）压入 tasks_ 队列。
     void Submit(std::function<void()> task) {
       {
         std::lock_guard<std::mutex> lock(mutex_);
         tasks_.push(std::move(task));
       }
       cond_.notify_one();  // 通知一个线程处理任务
     }
   
    private:
     void WorkerLoop() {
       while (true) {
         ;
         {
           std::unique_lock<std::mutex> lock(mutex_);
           cond_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
   
           if (stop_ && tasks_.empty()) {
             return;  // 停止且无任务时退出线程
           }
   
           task = std::move(tasks_.front());
           tasks_.pop();
         }
         task();  // 执行任务
       }
     }
   
     std::vector<std::thread> workers_;                     // 后台线程池
     std::queue<std::function<void()>> tasks_;              // 待执行任务队列
     std::mutex mutex_;                                     // 任务队列锁
     std::condition_variable cond_;                         // 用于线程等待/通知
     bool stop_;                                            // 线程池停止标志
   };
   
   