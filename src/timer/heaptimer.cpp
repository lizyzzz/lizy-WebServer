#include "heaptimer.h"


HeapTimer::HeapTimer() {
    heap_.reserve(64);
}

HeapTimer::~HeapTimer() {
    clear();
}

void HeapTimer::siftUp_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2; // 父节点
    while (i > 0) {
        if (heap_[j] < heap_[i]) {
            // 满足条件, 停止调整
            break;
        }
        swapNode_(i, j); 
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::siftDown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());

    size_t i = index;
    size_t j = i * 2 + 1; // 左子节点
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if (heap_[i] < heap_[j]) break;
        swapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}


void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());

    std::swap(heap_[i], heap_[j]);
    // 更新 ref_ 映射
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);

    size_t i;
    if (ref_.count(id) == 0) {
        /* 新节点: 堆尾插入, 调整堆*/
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftUp_(i);
    }
    else {
        /* 旧节点: 调整堆*/
        i = ref_.at(id);
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        // 向上或向下调整
        if (!siftDown_(i, heap_.size())) {
            siftUp_(i);
        }
    }
}

void HeapTimer::doWork(int id) {
    if (heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::del_(size_t index) {
    assert(!heap_.empty() && index >= 0 && index < heap_.size());

    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        swapNode_(i, n); // 把要删除的节点放在最后
        if (!siftDown_(i, n)) {
            // 向上或向下调整
            siftUp_(i);
        }
    }
    // 删除尾元素
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
    assert(!heap_.empty() && ref_.count(id) > 0);

    heap_[ref_.at(id)].expires = Clock::now() + MS(timeout);
    siftDown_(ref_[id], heap_.size());
}

void HeapTimer::tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        TimerNode node = heap_.front(); // 最小的超时节点
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            // 没有超时的节点
            break;
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    tick();
    int res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}