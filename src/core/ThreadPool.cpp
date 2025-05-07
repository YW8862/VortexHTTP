#include "core/ThreadPool.h"
#include "utils/Logger.h"

ThreadPool::ThreadPool(size_t numThreads) 
{
    LOG(INFO) << "Initializing thread pool with " << numThreads << " workers";
    
    // 创建一批工作线程
    for (size_t i = 0; i < numThreads; ++i) 
    {
        workers.emplace_back([this] {
            // 输出线程启动信息
            LOG(DEBUG) << "Worker thread started (ID: " 
                      << std::this_thread::get_id() << ")";
            
            // 持续检查任务队列，如果有任务就执行
            while (true) 
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    
                    // 等待条件：停止或队列非空
                    condition.wait(lock, [this] {
                        return stop.load() || !tasks.empty();
                    });
                    
                    // 终止条件：停止且队列为空
                    if (stop && tasks.empty()) 
                    {
                        LOG(DEBUG) << "Worker thread exiting (ID: " 
                                  << std::this_thread::get_id() << ")";
                        return;
                    }
                    
                    // 获取队列第一个任务
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                
                // 执行任务并记录日志
                LOG(DEBUG) << "Executing task (Worker ID: " 
                          << std::this_thread::get_id() << ")";
                try {
                    task();
                } catch (const std::exception& e) {
                    LOG(ERROR) << "Task failed: " << e.what();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() 
{
    LOG(INFO) << "Shutting down thread pool";
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    
    // 唤醒所有线程
    condition.notify_all();
    
    // 等待所有线程结束
    for (std::thread& worker : workers) 
    {
        if (worker.joinable()) 
        {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) 
{
    // 创建作用域，RAII自动控制上锁解锁
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // 如果线程池关闭，抛出运行时异常
        if (stop) 
        {
            LOG(ERROR) << "Enqueue on stopped ThreadPool";
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }
        // 否则加入任务队列
        tasks.emplace(std::move(task));
    }
    // 通知一个等待线程
    condition.notify_one();
}

size_t ThreadPool::queueSize() const 
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size();
}