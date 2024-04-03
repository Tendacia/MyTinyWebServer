//
// Created by tendacia on 24-4-3.
//

#ifndef MYTINYWEBSERVER_FIBER_H
#define MYTINYWEBSERVER_FIBER_H

#include<memory>
#include <functional>
#include <ucontext.h>






class Fiber : public  std::enable_shared_from_this<Fiber>{

    friend  class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief 协程状态
     */

    enum State{
        //初始化
        INIT,
        //暂停
        HOLD,
        //执行中
        EXEC,
        //结束
        TERM,
        //可执行
        READY,
        //异常
        EXCEPT
    };

private:
    /**
     * @brief 每个线程第一个协程构造
     */
     Fiber();

public:

    /**
     * @brif 构造函数
     * @param cb  协程执行的函数
     * @param stack_size  协程栈的大小
     * @param use_caller  是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stack_size = 0, bool use_caller = false);

    ~Fiber();

    /**
     * @brief 重置协程执行函数，并设置状态
     * @param cb
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     */
    void swapIn();

    /**
     * @brief  将当前协程切换到后台
     */
    void swapOut();

    /**
     * @brief 将当前协程切换到执行状态
     */
    void call();

    /**
     * @brief 将当前协程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */

    void back();

    /**
     * @brief 返回协程id
     * @return
     */

    uint64_t  getId() const {return m_id;}

    /**
     * @brief 返回协程状态
     * @return
     */

    State getState() const {return m_state;};


public:

    /**
     * @brief 设置当前线程的运行协程
     * @param f  运行协程
     */
    static void SetThis(Fiber * f);

    /**
     * @brief 返回当前所在的协程
     * @return
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 将当前协程切换到后台，并设置为READY状态
     * @post getState() = READY
     */
    static void YieldToReady();

    /**
     * @brief 将当前协程切换到后台，并设置为HOLD状态
     * @post getState() = HOLD
     */

    static void YieldToHold();

    /**
     * @brief 返回当前协程的总数量
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数，
     * @post 执行完之后返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 协程执行函数
     * @post 执行完成后返回到线程调度协程
     */
    static void CallerMainFunc();

    /**
     * @brief 获取当前协程id
     * @return
     */
    static uint64_t  GetFiberId();



private:
    //协程id
    uint64_t  m_id = 0;
    //协程运行栈大小
    uint32_t  m_stacksize = 0;
    //协程状态
    State m_state = INIT;
    //协程上下文
    ucontext_t m_ctx;
    // 协程运行栈指针
    void * m_stack = nullptr;
    // 协程运行函数
    std::function<void()> m_cb;
};


#endif //MYTINYWEBSERVER_FIBER_H
