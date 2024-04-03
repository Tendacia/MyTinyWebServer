//
// Created by tendacia on 24-4-3.
//

#include <atomic>
#include <cassert>
#include  <iostream>


#include "fiber.h"

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_thread_fiber = nullptr;
static  uint32_t g_fiber_stack_size  = 1024 * 128;


class MallocStackAllocator{
public:
    static void * Alloc(size_t size)
    {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size)
    {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;


Fiber::Fiber() {

    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx))
    {

        std::cerr << "Failed to get context!\n";
        assert(false);
    }

    ++s_fiber_count;
    std::cout << "Fiber::Fiber main\n";

}

Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool use_caller)
    : m_id(++s_fiber_id),
    m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stack_size ? stack_size : g_fiber_stack_size;

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(& m_ctx))
    {
        std::cerr << "Failed to getcontext!\n";
        assert(false);
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller)
    {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }
    else
    {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    std::cout << "Fiber::Fiber id=" << m_id << "\n";

}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack)
    {
        assert(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator ::Dealloc(m_stack, m_stacksize);
    }
    else
    {
        assert(m_cb);
        assert(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this)
        {
            SetThis(nullptr);
        }
    }

    std::cout << "Fiber::~Fiber id=" << m_id
                <<" total=" << s_fiber_count << '\n';

}



void Fiber::reset(std::function<void()> cb) {
    assert(m_stack);
    assert(m_state == TERM
            ||  m_state == EXCEPT
             || m_state == INIT);
    m_cb = cb;
    if(getcontext(&m_ctx))
    {
        std::cerr << "Failed to getcontext!\n";
        assert(false);
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;

}

void Fiber::swapIn() {
    SetThis(this);
    assert(m_state != EXEC);
    m_state = EXEC;

    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx))
    {
        std::cerr << "Failed to swapcontext!\n";
        assert(false);
    }
}

void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx))
    {
        std::cerr << "Failed to swapcontext!\n";
        assert(false);
    }


}

void Fiber::call() {

    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_thread_fiber->m_ctx, &m_ctx))
    {
        std::cerr << "Failed to swapcontext!\n";
        assert(false);
    }

}

void Fiber::back() {
    SetThis(t_thread_fiber.get());
    if(swapcontext(&m_ctx, &t_thread_fiber->m_ctx))
    {
        std::cerr << "Failed to swapcontext\n";
        assert(false);
    }
}

void Fiber::SetThis(Fiber *f) {
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {

    if(t_fiber)
    {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr  main_fiber(new Fiber);
    assert(t_fiber == main_fiber.get());
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    assert(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    assert(cur->m_state == EXEC);
    cur->swapOut();
}

uint64_t  Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {

    Fiber::ptr  cur = GetThis();
    assert(cur);

    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch (std::exception& ex)
    {
        cur->m_state = EXCEPT;
        std::cerr << "Fiber Except: " << ex.what()
                << "fiber_id=" << cur->getId()
                << std::endl;
    }
    catch(...)
    {
        cur->m_state = EXCEPT;
        std::cerr << "Fiber Except fiber_id=" << cur->getId() << std::endl;
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    std::cerr << "Never reach fiber_id=" << raw_ptr->getId() << std::endl;
    assert(false);
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    assert(cur);

    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& ex)
    {
        cur->m_state = EXCEPT;
        std::cerr << "Fiber Except: " << ex.what()
                    << " fiber_id=" << cur->getId() << std::endl;
    }
    catch(...)
    {
        cur->m_state = EXCEPT;
        std::cerr << "Fiber Except fiber_id=" << cur->getId() << std::endl;
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();

}

uint64_t Fiber::GetFiberId() {
    if(t_fiber)
    {
        return t_fiber->getId();
    }
    return 0;
}
