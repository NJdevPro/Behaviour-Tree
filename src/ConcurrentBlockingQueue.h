//
// Created by njanin on 16/11/22.
//


#ifndef _CONCURRENT_BLOCKING_QUEUE_H_
# define _CONCURRENT_BLOCKING_QUEUE_H_

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <cstring>

/*
* Thread safe concurrent blocking queue with a fixed capacity.
* It's a FIFO: items are popped out in the order they've been pushed in.
* The push operations are blocked when the queue is full.
*/
template <typename T>
class ConcurrentBlockingQueue
{
public:
    ConcurrentBlockingQueue(const size_t capacity = 10) : max_size_(capacity) {}

    T pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            std::stringstream msg;
            msg << "Can't pop : queue is empty !\n" << std::endl;
            std::cout << msg.str();
            queue_empty_.wait(mlock);
        }
        auto item = queue_.front();
        queue_.pop();
        mlock.unlock();
        queue_full_.notify_one();
        return item;
    }

    void pop(T& item)
    {
        {
            std::unique_lock<std::mutex> mlock(mutex_);
            while (queue_.empty())
            {
                std::stringstream msg;
                msg << "Can't pop : queue is empty !\n" << std::endl;
                std::cout << msg.str();
                queue_empty_.wait(mlock);
            }
            item = queue_.front();
            queue_.pop();
        }
        queue_full_.notify_one();
    }

    void push(const T& item)
    {
        {
            std::unique_lock<std::mutex> mlock(mutex_);
            while (queue_.size() >= max_size_) {
                std::stringstream msg;
                msg << "Can't push : queue is full !\n" << std::endl;
                std::cout << msg.str();
                queue_full_.wait(mlock);
            }
            queue_.push(item);
        }
        queue_empty_.notify_one();
    }

    void push(T&& item)
    {
        {
            std::unique_lock<std::mutex> mlock(mutex_);
            while (queue_.size() >= max_size_) {
                std::stringstream msg;
                msg << "Can't push : queue is full !\n" << std::endl;
                std::cout << msg.str();
                queue_full_.wait(mlock);
            }
            queue_.push(std::move(item));
        }
        queue_empty_.notify_one();
    }

    inline size_t size() noexcept
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.size();
    }

    inline bool empty() noexcept
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.empty();
    }

    inline bool full() noexcept
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.size() >= max_size_;
    }

private:
    std::queue<T> queue_;
    const size_t max_size_;
    std::mutex mutex_;
    std::condition_variable queue_full_;	// blocks when the queue is full
    std::condition_variable queue_empty_;	// blocks when the queue is empty
};


#endif /* !_CONCURRENT_BLOCKING_QUEUE_H_ */
