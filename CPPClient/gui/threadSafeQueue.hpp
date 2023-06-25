#ifndef _THREADSAFEQUEUE_
#define _THREADSAFEQUEUE_
// credit to https://codereview.stackexchange.com/questions/267847/thread-safe-message-queue for inspiration
#include <queue>
#include <memory>
#include <condition_variable>
template <typename T>
class ThreadSafeQueue {
public:
    void push(T&& item) {
        {
            std::lock_guard lock(mutex);
            queue.push(item);
        }

        cond_var.notify_one();
    }

    T& front() {
        std::unique_lock lock(mutex);
        cond_var.wait(lock, [&]{ return !queue.empty(); });
        return queue.front();
    }

    void pop() {
        std::lock_guard lock(mutex);
        queue.pop();
    }

    bool isEmpty()
    {
        return queue.empty();
    }

private:
    std::mutex mutex;
    std::condition_variable cond_var;
    std::queue<T> queue;
};
#endif //_THREADSAFEQUEUE_