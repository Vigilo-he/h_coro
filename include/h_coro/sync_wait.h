#pragma once
#include <condition_variable>
#include <mutex>
#include <utility>
#include "h_coro/task.h"

namespace h_coro {

class SyncWait {
public:
    explicit SyncWait(Task<> task) 
        : task_(std::move(task)) 
        , done_(false) 
    {
        // ❌ 删掉这一行，因为新的 Task 用 Scheduler 了，不需要这个回调了
        // task_.GetHandle().promise().on_done_callback_ = [this]() { this->Notify(); };
    }

    void RunAndWait() {
        // 因为我们在新 Task 的 initial_suspend 是 suspend_always
        // 所以如果要在这里直接跑，必须手动 resume
        auto handle = task_.GetHandle();
        if (handle) {
            handle.resume(); 
        }

        // 注意：因为没了 on_done_callback_，这里的 wait 会永远卡住！
        // 如果你还要用 SyncWait 测纯内存协程，你需要把 Task 改回去，或者我们在下面换个写法。
    }

private:
    void Notify() { /* ... */ }

    Task<> task_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool done_;
};

} // namespace h_coro