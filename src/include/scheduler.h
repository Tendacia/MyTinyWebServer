//
// Created by tendacia on 24-4-3.
//

#ifndef MYTINYWEBSERVER_SCHEDULER_H
#define MYTINYWEBSERVER_SCHEDULER_H

#include <list>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <atomic>

#include "fiber.h"





class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name= "");

    virtual  ~Scheduler();

    const std::string & GetName() const {return m_name;}

    void Start();

    void Stop();

    template<class FiberOrCb>
    void Schedule(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = false;
        {
           std::lock_guard<std::mutex> lock(m_mutex);
           need_tickle = ScheduleNoLock(fc, thread);

        }
        if(need_tickle)
        {
            Tickle();
        }
    }


    template<class InputIterator>
    void Schedule(InputIterator begin, InputIterator end)
    {
        bool need_tickle = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while(begin != end)
            {
                need_tickle = ScheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
            if(need_tickle)
            {
                tickle();
            }
        }
    }

    void SwitchTo(int thread = -1);
    std::ostream& Dump(std::ostream& os);

public:
    static Scheduler* GetThis();
    static Fiber* GetMainFiber();


protected:
    virtual  void Tickle();

    void Run();

    virtual  bool Stopping();

    virtual  void Idle();

    void SetThis();

    bool HasIdleThreads(){ return m_idle_thread_count > 0;}

private:
    template <class FiberOrCb>
    bool ScheduleNoLock(FiberOrCb fc, int thread)
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb)
        {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread{

        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f), thread(thr)
        {
        }

        FiberAndThread(Fiber::ptr *f, int thr)
            : thread(thr)
        {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr)
        {
        }

        FiberAndThread(): thread(-1)
        {
        }

        void Reset()
        {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };


private:
    // 互斥
    std::mutex m_mutex;
    //线程池
    std::vector<std::shared_ptr<std::thread> > m_threads;
    //待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    // use_caller为true时有效，调度协程
    Fiber::ptr m_root_fiber;
    //协程调度器名称
    std::string m_name;

protected:
    //协程下的线程id数组
    std::vector<int> m_threadIds;
    //线程数量
    size_t m_threadCount = 0;
    //工作线程数量
    std::atomic<size_t> m_active_thread_count{0};
    //空闲线程数量
    std::atomic<size_t> m_idle_thread_count{0};
    //是否正在停止
    bool m_stopping = true;
    //是否自动停止
    bool m_auto_stop = false;
    // 主线程id(use_caller)
    int m_root_thread = 0;

};


class SchedulerSwitcher{
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();

private:
    Scheduler* m_caller;
};


#endif //MYTINYWEBSERVER_SCHEDULER_H
