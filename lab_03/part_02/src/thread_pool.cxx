#include "thread_pool.hpp"

#include <iterator>
#include <algorithm>

ThreadPool::ThreadPool(std::size_t threadCount): terminate(false)
{
    if (threadCount == 0)
    {
        threadCount = std::thread::hardware_concurrency();
    }

    threads.reserve(threadCount);
    std::generate_n(std::back_inserter(threads), threadCount, [this]() { return std::thread{ threadTask, this }; });
}

ThreadPool::~ThreadPool()
{
    clear();
    terminate = true;
    jobsAvailable.notify_all();

    for (auto &t: threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

void ThreadPool::clear()
{
    std::lock_guard<std::mutex> lock{ jobsMutex };

    while (!jobs.empty())
    {
        jobs.pop();
    }
}

void ThreadPool::threadTask(ThreadPool* pool)
{
    while (!pool->terminate)
    {
        std::unique_lock<std::mutex> jobsLock{ pool->jobsMutex };

        if (pool->jobs.empty())
        {
            pool->jobsAvailable.wait(jobsLock, [&]() { return pool->terminate || !(pool->jobs.empty()); });
        }

        if (!pool->terminate)
        {
            auto job = std::move(pool->jobs.front());
            pool->jobs.pop();

            jobsLock.unlock();

            job();
        }
    }
}

