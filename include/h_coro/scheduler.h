#pragma once
#include <functional>

namespace h_coro {

// 这是一个接口，用来把协程的唤醒动作交给外部（比如 EventLoop）
class Scheduler {
public:
    virtual ~Scheduler() = default;
    
    // 外部调度器需要实现这个：把任务扔进它的事件循环里
    virtual void Schedule(std::function<void()> task) = 0;
};

} // namespace h_coro