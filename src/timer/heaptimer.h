/*
    最小堆实现的定时器
*/


#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <vector>
#include <unordered_map>
#include <time.h>
#include <chrono>
#include <functional>
#include <algorithm>
#include <assert.h>


typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

// 定时器的节点类型
struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer();

    /// @brief 调整指定id的节点的超时时间
    /// @param id 指定id
    /// @param timeout 新的超时时间 (单位:ms)
    void adjust(int id, int timeout);

    /// @brief 增加新的定时器节点
    /// @param id 节点id
    /// @param timeout 超时时间, 单位: ms
    /// @param cb 回调函数
    void add(int id, int timeout, const TimeoutCallBack& cb);

    /// @brief 删除指定 id 节点, 并执行回调函数
    /// @param id 指定的id
    void doWork(int id);

    /// @brief 清空定时器
    void clear();

    /// @brief 清空超时节点
    void tick();

    /// @brief 根节点出堆
    void pop();

    /// @brief 返回最近一个超时节点的超时时间
    /// @return 超时时间 (单位:ms)
    int GetNextTick();
    
private:
    /// @brief 删除指定位置的节点
    /// @param index 位置(数组下标)
    void del_(size_t index);

    /// @brief 向上调整过程
    /// @param i 向上调整的位置(数组下标)
    void siftUp_(size_t i);

    /// @brief 向下调整
    /// @param index 向上调整的位置(数组下标)
    /// @param n 数组最大边界
    /// @return 是否已经向下调整过
    bool siftDown_(size_t index, size_t n);

    /// @brief 交换两个节点位置
    /// @param i 左节点下标
    /// @param j 右节点下标
    void swapNode_(size_t i, size_t j);

    
private:
    std::vector<TimerNode> heap_;

    // 已有节点再插入时用到
    // key: TimerNode.id  value: vector.index
    std::unordered_map<int, size_t> ref_; 
};






#endif