//
// Created by kchen on 12/5/17.
//

#ifndef SPLUNK_HEC_CLIENT_CPP_THREAD_POOL_H
#define SPLUNK_HEC_CLIENT_CPP_THREAD_POOL_H

#include "blocking_queue.h"
#include "timeout_exception.h"

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <future>
#include <functional>


namespace concurrentcpp {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t size = std::max(std::thread::hardware_concurrency(), 2u),
                        const std::chrono::milliseconds& submit_timeout = std::chrono::milliseconds{1000})
            : tasks_(2 * size) {
        for (std::size_t i = 0; i < size; i++) {
            workers_.emplace_back(std::thread(std::bind(&ThreadPool::_wait_for_tasks, this)));
        }
    }

    ~ThreadPool() {
        stop_ = true;
        for (auto& th: workers_) {
            th.join();
        }
    }

    // Submit a task to thread pool, return std::future<result_type> if successfully submitting the task
    // Throws timeout_error when TimeoutException
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        bool success = tasks_.put([task](){(*task)();}, timedout_);
        if (!success) {
            throw TimeoutException("timed out when submitting task");
        }
        return res;
    };

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    void _wait_for_tasks() {
        while (!stop_.load()) {
            std::function<void()> task;
            bool got_task = tasks_.get(task, timedout_);
            if (got_task && task) {
                task();
            }
        }
    }

private:
    std::atomic_bool stop_;
    std::chrono::milliseconds timedout_;
    std::vector<std::thread> workers_;
    BlockingQueue<std::function<void()>> tasks_;
};

} // namespace concurrentcpp

#endif //SPLUNK_HEC_CLIENT_CPP_THREAD_POOL_H
