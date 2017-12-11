//
// Created by kchen on 12/5/17.
//

#ifndef SPLUNK_HEC_CLIENT_CPP_BLOCKING_QUEUE_H
#define SPLUNK_HEC_CLIENT_CPP_BLOCKING_QUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cassert>

namespace concurrentcpp {

template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t size): max_size_(size) {
    }

    bool put(const T& v, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{0}) {
        auto callback = [&]{
            q_.push_back(v);
        };
        return do_put(callback, timeout);
    }

    bool put(T&& v, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{0}) {
        auto callback = [&]{
            q_.push_back(std::move(v));
        };
        return do_put(callback, timeout);
    }

    bool get(T& v, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{0}) {
        {
            std::unique_lock <std::mutex> lock{mutex_};
            if (timeout.count() != 0) {
                if (!cv_.wait_for(lock, timeout, [&]{ return !q_.empty();})) {
                    // wait until there is element
                    // after timeout, there is still no element, return false;
                    return false;
                }
            } else {
                // wait forever
                cv_.wait(lock, [&]{ return !q_.empty();});
            }

            assert(!q_.empty());

            v = std::move(q_.front());
            q_.pop_front();
            lock.unlock();
        }

        cv_.notify_all();

        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return q_.empty();
    }

    std::size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return q_.size();
    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;
    BlockingQueue(BlockingQueue&&) = delete;
    BlockingQueue& operator=(BlockingQueue&&) = delete;

private:
    template <typename C>
    bool do_put(C& callback, const std::chrono::milliseconds& timeout) {
        {
            std::unique_lock <std::mutex> lock{mutex_};

            if (timeout.count() != 0) {
                if (!cv_.wait_for(lock, timeout, [&]{ return q_.size() < max_size_; })) {
                    // wait until there is element
                    // after timeout, there is still no element, return false;
                    return false;
                }
            } else {
                // wait forever
                cv_.wait(lock, [&]{ return q_.size() < max_size_; });
            }

            assert(q_.size() < max_size_);

            callback();
            lock.unlock();
        }
        cv_.notify_all();
        return true;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<T> q_;
    std::size_t max_size_;
};

} // concurrentcpp

#endif //SPLUNK_HEC_CLIENT_CPP_BLOCKING_QUEUE_H
