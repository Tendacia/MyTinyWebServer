//
// Created by tendacia on 24-4-3.
//

#include <cassert>
#include <unistd.h>
#include <syscall.h>

#include "scheduler.h"

static thread_local  Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr;


pid_t  GetThreadId()
{
    return syscall(SYS_gettid);
}




Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    :m_name(name)
{
    assert(threads > 0);

    if(use_caller)
    {
        Fiber::GetThis();
        --threads;

        assert(GetThis() == nullptr);
        t_scheduler = this;

        m_root_fiber.reset(new Fiber(std::bind(&Scheduler::Run, this), 0, true));

        t_scheduler_fiber = m_root_fiber.get();
        m_root_thread = GetThreadId();
        m_threadIds.push_back(m_root_thread);
    }
    else
    {
        m_root_thread = -1;
    }

    m_threadCount = threads;
}

Scheduler::~Scheduler() {

    assert(m_stopping);
    if(GetThis() == this)
    {
        t_scheduler = nullptr;
    }
}

void Scheduler::Start() {

    std::lock_guard<std::mutex> lock(m_mutex);
    if(!m_stopping)
    {
        return;
    }

    m_stopping = false;
    assert(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i)
    {
        m_threads[i].reset(new std::thread(std::bind(&Scheduler::Run, this), m_name + "_" + std::to_string(i)));
        // implement a method to get the thread id
        // m_threadIds.push_back(GetThreadId());
    }
}

void Scheduler::Stop() {

    m_auto_stop = true;
    if(m_root_fiber && m_threadCount == 0
        && (m_root_fiber->getState() == Fiber::TERM ||
            m_root_fiber->getState() == Fiber::INIT))
    {
        std::cout <<  m_name << " stopped\n";
        m_stopping = true;

        if(Stopping())
        {
            return ;
        }
    }

    if(m_root_thread != -1)
    {
        assert(GetThis() == this);
    }
    else
    {
        assert(GetThis() != this);
    }

    m_stopping = true;

    for(size_t i = 0; i < m_threadCount; ++i)
    {
        Tickle();
    }

    if(m_root_fiber)
    {
        Tickle();
    }

    if(m_root_fiber)
    {
        if(!Stopping())
        {
            m_root_fiber->call();
        }
    }

    std::vector<std::shared_ptr<std::thread> > thrs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs)
    {
        i->join();
    }

}



void Scheduler::SwitchTo(int thread) {
    assert(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this)
    {
        if(thread == -1 || thread == GetThreadId())
        {
            return;
        }
    }
    Schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream &Scheduler::Dump(std::ostream &os) {
    os << "[Scheduler name=" << m_name
        << " size=" << m_threadCount
        << " active_count=" << m_active_thread_count
        << " idle_count=" << m_idle_thread_count
        << " stopping=" << m_stopping
        << " ]" << std::endl << " ";
    for(size_t i = i; i < m_threadIds.size(); ++i)
    {
        if(i)
        {
            os <<", ";
        }
        os << m_threadIds[i];
    }

    return os;
}

Scheduler *Scheduler::GetThis() {
    return nullptr;
}

Fiber *Scheduler::GetMainFiber() {
    return nullptr;
}

void Scheduler::Tickle() {
    std::cout << "Tickle\n";
}

void Scheduler::Run() {

    std::cout << m_name << " run\n";
    // set_hook_enable(true)
    SetThis();
    if(GetThreadId() != m_root_thread)
    {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::Idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true)
    {
        ft.Reset();
        bool tickle_me = false;
        bool is_active = false;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end())
            {
                if(it->thread != -1 && it->thread != GetThreadId())
                {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                assert(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC)
                {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_active_thread_count;
                is_active  = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        if(tickle_me)
        {
            Tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM)
            && ft.fiber->getState() != Fiber::EXCEPT)
        {
            ft.fiber->swapIn();
            --m_active_thread_count;

            if(ft.fiber->getState() == Fiber::READY)
            {
                Schedule(ft.fiber);
            }
            else if(ft.fiber->getState() != Fiber::TERM
                && ft.fiber->getState() != Fiber::EXCEPT)
            {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.Reset();
        }
        else if(ft.cb)
        {
            if(cb_fiber)
            {
                cb_fiber->reset(ft.cb);
            }
            else
            {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.Reset();
            cb_fiber->swapIn();
            --m_active_thread_count;
            if(cb_fiber->getState() == Fiber::READY)
            {
                Schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM)
            {
                cb_fiber->reset(nullptr);
            }
            else
            {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        }
        else
        {
            if(is_active)
            {
                --m_active_thread_count;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM)
            {
                std::cout << "Idle fiber term\n";
                break;
            }

            ++m_idle_thread_count;
            idle_fiber->swapIn();
            --m_idle_thread_count;
            if(idle_fiber->getState() != Fiber::TERM
                && idle_fiber->getState() != Fiber::EXCEPT)
            {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }

    }
}

bool Scheduler::Stopping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_auto_stop && m_stopping
        && m_fibers.empty() && m_active_thread_count == 0;
}

void Scheduler::Idle() {
    std::cout << "Idle\n";
    while (!Stopping())
    {
        Fiber::YieldToHold();
    }
}

void Scheduler::SetThis() {
    t_scheduler = this;
}


SchedulerSwitcher::SchedulerSwitcher(Scheduler *target)
{
    m_caller = Scheduler::GetThis();
    if(target)
    {
        target->SwitchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher()
{
    if(m_caller)
    {
        m_caller->SwitchTo();
    }
}