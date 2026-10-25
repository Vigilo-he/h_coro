#pragma once
#include <coroutine>
#include <exception>
#include "h_coro/scheduler.h" // 引入接口

namespace h_coro {

template<typename T = void>
class Task {
public:
    struct promise_type {
        std::coroutine_handle<> continuation_{nullptr};
        std::exception_ptr exception_{nullptr};
        
        // 记录外部调度器
        Scheduler* scheduler_{nullptr}; 

        Task get_return_object() {
            return Task(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        // 注意：这里改成 suspend_always。如果外部有调度器，就让调度器去唤醒它
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct FinalAwaiter {
            bool await_ready() const noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) const noexcept {
                if (h.promise().continuation_) {
                    return h.promise().continuation_;
                }
                return std::noop_coroutine();
            }
            void await_resume() const noexcept {}
        };

        FinalAwaiter final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { exception_ = std::current_exception(); }
    };

    explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
    ~Task() { if (handle_) handle_.destroy(); }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

    bool await_ready() const noexcept { return false; }
    
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        handle_.promise().continuation_ = caller;
        return handle_;
    }
    
    void await_resume() {
        if (handle_.promise().exception_) {
            std::rethrow_exception(handle_.promise().exception_);
        }
    }

    // 👇 核心：把这个协程绑定到调度器上并启动
    void ScheduleOn(Scheduler* scheduler) {
        if (handle_) {
            handle_.promise().scheduler_ = scheduler;
            // 把协程的 resume 动作丢给外部调度器
            scheduler->Schedule([h = handle_]() {
                h.resume();
            });
        }
    }

    std::coroutine_handle<promise_type> GetHandle() const { return handle_; }

private:
    std::coroutine_handle<promise_type> handle_;
};

} // namespace h_coro