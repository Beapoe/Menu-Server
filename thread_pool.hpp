#ifndef THREAD_POOL_HPP_
#define THREAD_POOL_HPP_

#include <queue>
#include <thread>
#include <memory>
#include <mutex>
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <iostream>
#include <any>

// Non-Windows only
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ThreadPool {
    struct Task
    {
        TaskFunction func;
        std::string name;
        std::any params;
        std::any results;
    };

using TaskFunction = std::function<std::any()>;

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool() = default;
    void Delete();
    static std::unique_ptr<ThreadPool> getInstance();

    void addTask(Task task);

private:
    Task run(int id);
    static inline std::unique_ptr<ThreadPool> instance = nullptr;
    std::queue<Task> tasks;
    static std::vector<std::any> results;
    std::shared_ptr<std::mutex> tasksLock = std::make_shared<std::mutex>();
    std::vector<std::unique_ptr<std::thread>> threads = std::vector<std::unique_ptr<std::thread>>();
    std::atomic<bool> running{true};
    std::atomic<int> task_num{0};
};

ThreadPool::ThreadPool()
{
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int CPUCore_num = sysInfo.dwNumberOfProcessors;
#else
    int CPUCore_num = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    for (int current = 0; current < (CPUCore_num > 8 ? 8 : CPUCore_num); current++)
    {
        auto function = std::bind(ThreadPool::run, this, current);
        threads.push_back(std::make_unique<std::thread>(function));
    }
}

inline void ThreadPool::Delete()
{
    running.store(false);
    for (int i = 0; i < threads.size(); i++)
    {
        task_num.fetch_add(1);
    }
    for (const auto &thread : threads)
    {
        thread->join();
    }
    task_num.store(0);
}

inline std::unique_ptr<ThreadPool> ThreadPool::getInstance()
{
    if (instance == nullptr)
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr)
            instance = std::make_unique<ThreadPool>();
    } // 双重检查，防止多个实例被创建
    return std::move(instance);
}

inline void ThreadPool::addTask(Task task)
{
    {
        {
            std::lock_guard<std::mutex> lock(*tasksLock); // 当一个线程想要添加任务到tasks队列时，它首先需要获取tasksLock的锁。如果锁已经被其他线程持有，那么当前线程将被阻塞，直到锁被释放。这样就可以确保同一时间只有一个线程可以修改tasks队列，从而避免了数据竞争和数据不一致的问题。
            tasks.push(task);
        }
        task_num.fetch_add(1);
    }
}

inline Task ThreadPool::run(int id)
{
    
    std::cout << "Thread:" << id << " is running" << std::endl;
    while (true)
    {
        while (true)
        {
            if (task_num.load() > 0)
            {
                if (!running)
                {
                    {
                        std::lock_guard<std::mutex> lock(*tasksLock);
                        if(task_num.load() > 0){
                            Task task;
                            task = tasks.front();
                            tasks.pop();
                            std::cout << "Work:" << task.name << " started" << std::endl;
                            task.results = task.func();
                            std::cout << "Work:" << task.name << " finished" << std::endl;
                            task_num.fetch_sub(1);
                            return task;
                        }
                    }
                }
                goto end;
            }
        }
    }
end:
    std::cout << "Thread:" << id << " finished" << std::endl
              << std::endl;
}
}

#endif