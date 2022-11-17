#pragma once
#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iostream>
#include <cstring>
#include <chrono>


/*
* Thread safe concurrent Stack with a fixed capacity.
* It's a LIFO: items are popped out in the inverse order they've been pushed in.
* The push operations are blocked when the queue is full.
*/
template <typename T>
class ConcurrentStack
{
public:
    /**
     * Constructor
     * @param capacity if negative, the stack is only limited by the available memory
     */
    ConcurrentStack(const size_t capacity = 10) :
            max_size_(capacity), bounded_(capacity > 0) {}
    ConcurrentStack(const size_t capacity, const std::chrono::milliseconds ms) :
            max_size_(capacity), bounded_(capacity > 0), timeout_(ms) {}

    ConcurrentStack(const ConcurrentStack& rhs){
        std::lock_guard<std::mutex> lock(mutex_);
        stack_ = rhs.stack_;
        bounded_ = rhs.bounded_;
        max_size_ = rhs.max_size_;
    }

    ConcurrentStack<T>& operator=(const ConcurrentStack<T>& rhs) {
        std::lock_guard<std::mutex> mlock(mutex_);
        if (this == &rhs) return *this;

        while(!stack_.empty()) { pop(); }
        bounded_ = rhs.bounded_;
        max_size_ = rhs.max_size_;
        stack_ = rhs.stack_;
        return *this;
    }

    T top() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (stack_.empty()){
            std::cout << "Can't read : queue is empty !" << std::endl;
            if(timeout_ == std::chrono::milliseconds(0))
                queue_empty_.wait(mlock);
            else
                queue_empty_.wait_for(mlock, timeout_);
        }
        auto item = stack_.top();
        mlock.unlock();
        queue_full_.notify_one();
        return item;
    }

    T pop(){
        std::unique_lock<std::mutex> mlock(mutex_);
        while (stack_.empty()) {
            std::cout << "Can't pop : queue is empty !" << std::endl;
            if(timeout_ == std::chrono::milliseconds(0))
                queue_empty_.wait(mlock);
            else
                queue_empty_.wait_for(mlock, timeout_);
        }
        auto item = stack_.top();
        stack_.pop();
        mlock.unlock();
        queue_full_.notify_one();
        return item;
    }

    void push(const T&& item) {
        {
            std::unique_lock<std::mutex> mlock(mutex_);
            while (bounded_ && stack_.size() >= max_size_) {
                std::cout  << "Can't push : queue is full !\n" << std::endl;
                queue_full_.wait_for(mlock, timeout_);
            }
            stack_.emplace(std::move(item));
        }
        queue_empty_.notify_one();
    }

    inline size_t size() noexcept{
        std::lock_guard<std::mutex> mlock(mutex_);
        return stack_.size();
    }

    inline bool is_empty() noexcept {
        std::lock_guard<std::mutex> mlock(mutex_);
        return stack_.empty();
    }

    inline bool is_full() noexcept {
        std::lock_guard<std::mutex> mlock(mutex_);
        return bounded_ && stack_.size() >= max_size_;
    }

private:
    std::stack<T> stack_;
    size_t max_size_;
    bool bounded_;
    std::chrono::milliseconds timeout_{0};
    std::mutex mutex_{};
    std::condition_variable queue_full_{};	// blocks when the stack is full
    std::condition_variable queue_empty_{};	// blocks when the stack is empty
};
