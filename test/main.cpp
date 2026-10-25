#include <iostream>
#include <coroutine>

namespace h_coro {

class Task {
public:
    struct promise_type {
        // 保存上一层调用我的协程句柄，为了执行完后唤醒它
        std::coroutine_handle<> caller{nullptr}; 

        Task get_return_object() { 
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; 
        }
        std::suspend_always initial_suspend() { return {}; }
        
        // 自定义 FinalAwaiter，用于实现“对称转移”（防止嵌套爆栈）
        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                // 如果谁在等我，我直接跳回去执行它（对称转移，不是压栈！）
                if (h.promise().caller) {
                    return h.promise().caller;
                }
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        FinalAwaiter final_suspend() noexcept { return {}; }

        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
    ~Task() { if (handle_) handle_.destroy(); }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // 移动构造（允许返回 Task）
    Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

    // Awaiter：当外部 co_await 这个 Task 时
    bool await_ready() const { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        handle_.promise().caller = caller; // 记下调用者
        return handle_; // 对称转移：立刻去执行当前 Task
    }
    void await_resume() {}

    // 暴露给外部手动启动用的接口
    std::coroutine_handle<promise_type> GetHandle() const { return handle_; }

private:
    std::coroutine_handle<promise_type> handle_;
};

Task ChildTask() {
    std::cout << "  [Child] 开始" << std::endl;
    co_await std::suspend_always{}; // 挂起，回到调用者
    std::cout << "  [Child] 恢复，结束" << std::endl;
}

Task ParentTask() {
    std::cout << "[Parent] 开始" << std::endl;
    co_await ChildTask(); // 嵌套
    std::cout << "[Parent] 结束" << std::endl;
}

} // namespace h_coro

int main() {
    std::cout << "--- 协程测试 ---" << std::endl;
    
    auto task = h_coro::ParentTask(); // 创建协程（未运行）
    
    // 手动启动它
    auto handle = task.GetHandle();
    handle.resume(); // 运行到第一个 co_await 挂起处，控制权回到 main
    
    std::cout << "--- main 重新拿回控制权 ---" << std::endl;
    
    // 再次恢复它（模拟调度器唤醒）
    handle.resume(); 
    
    std::cout << "--- 执行结束 ---" << std::endl;
    return 0;
}