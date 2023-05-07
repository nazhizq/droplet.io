#pragma once

#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <iostream>
#include <chrono>
#include "concurrent_queue.h"

namespace sparrow {

template <typename T>
class ConcurrentQueue  
{
public:
    ConcurrentQueue(int max_len = 10240) : 
        _max_len(max_len) {
    }
    size_t size() {
        std::unique_lock<std::mutex> lock(_mx);
        return _q.size();
    }
    void enqueue(const T val) {
        std::unique_lock<std::mutex> lock(_mx); 
        while (_q.size() == _max_len) {
            _cv.wait(lock);
        }
        _q.push_back(val);  //入队
        _cv.notify_all();  //唤醒
    } 
    T pop() {
        std::unique_lock<std::mutex> lock(_mx);
        while (_q.empty()) {
            _cv.wait(lock);
        }
        T val = _q.front();  //出队函数
        _q.pop_front();  //队头出，队尾加
        _cv.notify_all();
        return val;
    }
private:
    int _max_len;
    std::deque<T> _q;
    std::mutex _mx;   //全局锁
    std::condition_variable _cv;  //条件变量cv
};

}