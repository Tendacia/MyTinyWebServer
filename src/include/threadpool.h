//
// Created by tendacia on 3/1/24.
//

/**
 * @brief 线程池
 */

#ifndef MYTINYWEBSERVER_THREADPOOL_H
#define MYTINYWEBSERVER_THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "locker.h"
#include "sql_connection_pool.h"


template<typename T>
class threadPool {
public:
    //thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的，等待处理的请求的数量
    threadPool(int actor_model, connection_pool *connPool, int thread_number = 0, int max_request = 10000);

    ~threadPool();

    bool append(T *request, int state);

    bool append_p(T *request);

private:
    //工作线程运行的函数，它不断从工作队列中取出任务并执行之
    static void *worker(void *arg);

    void run();

private:
    int m_thread_number; //线程池中的线程数量
    int m_max_requests; //请求队列中允许的最大请求数
    pthread_t *m_threads; //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queue_locker; //保护请求队列的互斥锁
    sem m_queuestat; //是否有任务需要处理
    connection_pool *m_connPool; //数据库
    int m_actor_model; //模型切换
};

//创建线程池，并后台运行
template<typename T>
threadPool<T>::threadPool(int actor_model, connection_pool *connPool, int thread_number, int max_request)
        : m_actor_model(actor_model),
          m_thread_number(thread_number), m_max_requests(max_request), m_threads(NULL), m_connPool(connPool) {
    if (thread_number <= 0 || max_request <= 0) {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }

    for (int i = 0; i < thread_number; ++i) {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadPool<T>::~threadPool() {
    delete[] m_threads;
}


template<typename T>
bool threadPool<T>::append(T *request, int state) {
    m_queue_locker.lock();
    if (m_workqueue.size() >= m_max_requests) {
        m_queue_locker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queue_locker.unlock();
    m_queuestat.post();
    return true;
}


template<typename T>
bool threadPool<T>::append_p(T *request) {
    m_queue_locker.lock();
    if (m_workqueue.size() >= m_max_requests) {
        m_queue_locker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queue_locker.unlock();
    m_queuestat.post();
    return true;

}


template<typename T>
void *threadPool<T>::worker(void *arg) {
    threadPool *pool = (threadPool *) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadPool<T>::run() {
    while (true) {
        m_queuestat.wait();
        m_queue_locker.lock();
        if (m_workqueue.empty()) {
            m_queue_locker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queue_locker.unlock();

        if (!request) {
            continue;
        }

        if (m_actor_model == 1) {
            if (request->m_state == 0) {
                if (request->read_once()) {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else {
                if (request->write()) {
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }

        } else {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}


#endif //MYTINYWEBSERVER_THREADPOOL_H
