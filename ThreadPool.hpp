#include <iostream>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>
#include <memory>

using namespace std;
using TaskFunction = function<void()>;

namespace ThreadPool{
    // 任务结构体
struct Task
{
    TaskFunction func;
    string name;
};

// 线程池
class ThreadPool
{
public:
    static unique_ptr<ThreadPool> getInstance(){
        if (instance == nullptr)
        {
            static std::mutex mutex;
            std::lock_guard<std::mutex> lock(mutex);
            if (instance == nullptr)
                instance = std::make_unique<ThreadPool>();
        } // 双重检查，防止多个实例被创建
        return move(instance);
    }
    ThreadPool()
    {
        for (unsigned int i = 0; i < (thread::hardware_concurrency() > 8 ? 8 : thread::hardware_concurrency()); ++i)
        {
            threads.push_back(thread(&ThreadPool::run, this));
        }
    }

    ~ThreadPool()
    {
        stop.store(true);
        runCV.notify_all();
        for(auto &t : threads) t.join();// 未知原因阻塞主线程
    }

    template <class F, class... Args>
    auto addTask(string name, F &&f, Args &&...args) -> future<typename result_of<F(Args...)>::type>
    {
        using returnType = typename result_of<F(Args...)>::type;
        auto task = make_shared<packaged_task<returnType()>>(bind(forward<F>(f), forward<Args>(args)...));
        future<returnType> res = task->get_future();
        {
            unique_lock<mutex> lock(tasksLock);
            if (stop)
            {
                throw runtime_error("enqueue on stopped ThreadPool");
            }else tasks.push_back(Task{[task](){ (*task)(); }, name});
            lock.unlock();
        }
        runCV.notify_one();
        return res;
    }

private:
    void run()
    {
        while (true)
        {
            if (!stop.load())
            {
                unique_lock<mutex> lock(tasksLock);
                runCV.wait(lock, [this](){ return !tasks.empty();});
                TaskFunction task;
                string name;
                {
                    lock_guard<mutex> lock(tasksLock);
                    task = tasks.front().func;
                    name = tasks.front().name;
                    tasks.pop_front();
                }
                task();
                {
                    unique_lock<mutex> lock(tasksLock);
                    cout<<"Task: "<<name<<" completed"<<endl;
                    lock.unlock();
                }
            }else{
                cout<<this_thread::get_id()<<"finished"<<endl;
                exit(0);
            }
        }
    }

    static inline unique_ptr<ThreadPool> instance = nullptr;
    vector<thread> threads;
    deque<Task> tasks;
    mutex tasksLock;
    condition_variable runCV;
    atomic<bool> stop{false};
};
}

// int main()
// {
//     ThreadPool pool;

//     // 无返回值任务
//     auto f1 = []
//     { cout << "Hello from f1" << endl; };
//     pool.addTask("test1", f1);

//     // 有返回值任务
//     auto f2 = [](int a)
//     { return a * 2; };
//     auto res = pool.addTask("test2", f2, 5);
//     cout << "Result: " << res.get() << endl;
//     pool.~ThreadPool();
//     return 0;
// }