#ifndef DBR_CC_THREAD_POOL_HPP
#define DBR_CC_THREAD_POOL_HPP

#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <vector>
#include <functional>
#include <condition_variable>
#include <queue>


class ThreadPool
{
public:
    ThreadPool(std::size_t threadCount = 0);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = default;
    ThreadPool& operator=(ThreadPool&&) = default;
 
    ~ThreadPool();

    template<typename Func, typename... Args>
    auto add(Func&& func, Args&&... args)->std::future<typename std::result_of<Func(Args...)>::type>;

    void clear();

private:
    using Job = std::function<void()>;

    static void threadTask(ThreadPool* pool);

    std::queue<Job> jobs;
    mutable std::mutex jobsMutex;

    std::condition_variable jobsAvailable;

    std::vector<std::thread> threads;

    std::atomic<bool> terminate;
};

template<typename Func, typename... Args>
auto ThreadPool::add(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>
{
    using PackedTask = std::packaged_task<typename std::result_of<Func(Args...)>::type()>;

    auto task = std::make_shared<PackedTask>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

    auto ret = task->get_future();

    std::lock_guard<std::mutex> lock{ jobsMutex };
    jobs.emplace([task]() { (*task)(); });

    jobsAvailable.notify_one();

    return ret;
}

#endif