#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @brief 线程池实现
 * 
 * 基于任务队列的生产者-消费者模型实现
 * 支持动态线程数量控制
 */
class ThreadPool 
{
public:
    // 构造函数指定线程数量
    explicit ThreadPool(size_t numThreads);
    
    // 析构函数等待所有线程结束
    ~ThreadPool();
    
    // 向任务队列添加任务
    void enqueue(std::function<void()> task);
    
    // 获取当前任务队列大小
    size_t queueSize() const;

private:
    std::vector<std::thread> workers;       // 工作线程集合
    mutable std::mutex queueMutex;          // 任务队列互斥锁
    std::condition_variable condition;      // 条件变量
    std::queue<std::function<void()>> tasks;// 任务队列
    std::atomic<bool> stop{false};          // 停止标志
};