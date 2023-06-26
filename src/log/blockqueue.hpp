/*
    阻塞队列

*/


#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <vector>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockQueue {
public:
    explicit BlockQueue(size_t MaxCapacity = 1000);

    ~BlockQueue();
    /// @brief 清空操作
    void clear();
    /// @brief 是否为空
    /// @return true - 空, false - 非空
    bool empty();
    /// @brief 是否已满
    /// @return true - 满, false - 未满
    bool full();
    /// @brief 关闭队列
    void Close();
    /// @brief 返回当前队列长度
    /// @return 长度
    size_t size();
    /// @brief 返回当前队列的容量
    /// @return 容量
    size_t capacity();

    /// @brief 返回队头元素
    /// @return 队头元素
    T front();
    /// @brief 返回队尾元素
    /// @return 队尾元素
    T back();
    /// @brief 在队尾插入元素
    /// @param item 元素对象
    void push_back(const T& item);
    /// @brief 在队头插入元素
    /// @param item 元素对象
    void push_front(const T& item);

    /// @brief 从队头出队
    /// @param item 出队的元素
    /// @return 是否成功
    bool pop(T& item);
    /// @brief 带超时功能的从队头出队
    /// @param item 出队的元素
    /// @param timeout 超时时间, 单位 s
    /// @return 是否成功
    bool pop(T& item, int timeout);

    /// @brief 刷盘一次
    void flush();

    std::vector<T> GetAllData();

private:

    std::deque<T> deq_;

    size_t capacity_;

    std::mutex mtx_;

    bool isClose_;

    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;

};

template<class T> 
BlockQueue<T>::BlockQueue(size_t MaxCapacity) : capacity_(MaxCapacity), isClose_(false) { 
    assert(MaxCapacity > 0);
}

template<class T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<class T>
void BlockQueue<T>::Close() {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
}

template<class T>
void BlockQueue<T>::flush() {
    condConsumer_.notify_one();
}

template<class T>
void BlockQueue<T>::clear() {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        deq_.clear();
    }
}

template<class T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> lk(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockQueue<T>::full() {
    std::lock_guard<std::mutex> lk(mtx_);
    return deq_.size() >= capacity_;
}

template<class T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> lk(mtx_);
    return deq_.size();
}

template<class T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> lk(mtx_);
    return capacity_;
}

template<class T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> lk(mtx_);
    return deq_.front();
}

template<class T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> lk(mtx_);
    return deq_.back();
}

template<class T>
void BlockQueue<T>::push_back(const T& item) {
    {
        std::unique_lock<std::mutex> lk(mtx_);
        condProducer_.wait(lk, [this]() {
            return deq_.size() < capacity_;
        });
        deq_.push_back(item);
        condConsumer_.notify_one();
    }
}

template<class T>
void BlockQueue<T>::push_front(const T& item) {
    {
        std::unique_lock<std::mutex> lk(mtx_);
        condProducer_.wait(lk, [this]() {
            return deq_.size() < capacity_;
        });
        deq_.push_front(item);
        condConsumer_.notify_one();
    }
}

template<class T>
bool BlockQueue<T>::pop(T& item) {
    {
        std::unique_lock<std::mutex> lk(mtx_);
        condConsumer_.wait(lk, [this]() {
            return (deq_.size() > 0 || isClose_);
        });
        if (isClose_) {
            return false;
        }
        item = deq_.front();
        deq_.pop_front();
        condProducer_.notify_one();
    }
    return true;
}

template<class T>
bool BlockQueue<T>::pop(T& item, int timeout) {
    {
        std::unique_lock<std::mutex> lk(mtx_);
        if (condConsumer_.wait_for(lk, std::chrono::seconds(timeout), [this]() {
                return (deq_.size() > 0 || isClose_);
            }) == false ) {
            // 超时
            return false;
        }
        if (isClose_) {
            return false;
        }
        item = deq_.front();
        deq_.pop_front();
        condProducer_.notify_one();
    }
    return true;
}

template<class T>
std::vector<T> BlockQueue<T>::GetAllData() {
    std::vector<T> result;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        result.assign(deq_.begin(), deq_.end());
        deq_.clear();
    }
    return result;
}


#endif
