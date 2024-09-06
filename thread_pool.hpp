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
#include <condition_variable>
#include <future>

// Non-Windows only
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ThreadPool
{

    using TaskFunction = std::function<std::any(std::any)>;
    struct Task
    {
        TaskFunction func;
        std::string name;
        std::any params;
        std::promise<std::any> result_ptr;

        Task() = default;
        Task(TaskFunction func, std::string name, std::any params, std::promise<std::any> reuslt_ptr) : func(func), name(name), params(params), result_ptr(std::move(reuslt_ptr)) {}

        Task(Task &&other) noexcept
        {
            func = std::move(other.func);
            name = std::move(other.name);
            params = std::move(other.params);
            result_ptr = std::move(other.result_ptr);
        }

        Task &operator=(Task &&other) noexcept
        {
            func = std::move(other.func);
            name = std::move(other.name);
            params = std::move(other.params);
            result_ptr = std::move(other.result_ptr);
            return *this;
        }
        Task(const Task &other){
            
        }
        Task &operator=(const Task &other) = delete;
    };
    class ThreadPool
    {
    public:
        ThreadPool();
        ~ThreadPool() = default;
        void Delete();
        static std::unique_ptr<ThreadPool> getInstance();

        void addTask(Task task);

    private:
        void run(int id);
        static inline std::unique_ptr<ThreadPool> instance = nullptr;
        std::queue<Task> tasks;
        std::shared_ptr<std::mutex> tasksLock = std::make_shared<std::mutex>();
        std::vector<std::unique_ptr<std::thread>> threads = std::vector<std::unique_ptr<std::thread>>();
        std::atomic<bool> running{true};
        std::atomic<int> task_num{0};
        std::condition_variable getNewTask;
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
                std::lock_guard<std::mutex> lock(*tasksLock);
                // 当一个线程想要添加任务到tasks队列时，它首先需要获取tasksLock的锁。如果锁已经被其他线程持有，那么当前线程将被阻塞，直到锁被释放。这样就可以确保同一时间只有一个线程可以修改tasks队列，从而避免了数据竞争和数据不一致的问题。
                Task newTask(std::move(task));
                tasks.push(newTask);
            }
            task_num.fetch_add(1);
            getNewTask.notify_one();
        }
    }

    inline void ThreadPool::run(int id)
    {
        if (!running)
        {
            std::cout << "Thread:" << id << " finished" << std::endl
                      << std::endl;
        }
        else
        {
            std::unique_lock<std::mutex> lock(*tasksLock);
            bool getNewTaskFlag = task_num.load() > 0 && tasks.size() > 0 && running;
            getNewTask.wait(lock,[getNewTaskFlag](){return getNewTaskFlag;});
            {
                std::cout << "Thread:" << id << " is running" << std::endl;
                std::lock_guard<std::mutex> lock(*tasksLock);
                if (tasks.size() > 0)
                {
                    Task task = std::move(tasks.front());
                    tasks.pop();
                    std::cout << "Work:" << task.name << " started" << std::endl;
                    task.result_ptr.set_value(task.func(task.params));
                    task_num.fetch_sub(1);
                    std::cout << "Work:" << task.name << " finished" << std::endl;
                }
            }
        }
    }
}

#endif