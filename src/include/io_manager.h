//
// Created by tendacia on 24-4-6.
//

#ifndef MYTINYWEBSERVER_IO_MANAGER_H
#define MYTINYWEBSERVER_IO_MANAGER_H

#include "scheduler.h"
#include "rwlatch.h"


class IOManager : public  Scheduler{

public:
    typedef  std::shared_ptr<IOManager> ptr;

    /**
     * @brief IO事件
     */
    enum Event{
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4,
    };



private:

    /**
     * @brief Socket事件上下文类
     */
    struct FdContext{

        /**
         * @brief 事件上下文类
         */
        struct EventContext{
            //事件执行的调度器
            Scheduler* scheduler = nullptr;
            //事件协程
            Fiber::ptr fiber;
            //事件的回调函数
            std::function<void()> cb;
        };

        /**
         * @brief 获取事件上下文
         * @param event 事件类型
         * @return 返回对应事件的上下文
         */
        EventContext& GetContext(Event event);

        /**
         * @brief 重置事件上下文
         * @param[in,out] ctx 待重置的上下文类
         */
        void ResetContext(EventContext& ctx);

        /**
         * @brief 触发事件
         * @param event 事件类型
         */
        void TriggerEvent(Event event);

        //读事件上下文
        EventContext read;
        //写事件上下文
        EventContext write;
        //事件关联的句柄
        int fd = 0;
        //当前事件
        Event events = NONE;
        //事件的mutex
        std::mutex mutex;
    };



public:

    /**
     * @brief 构造函数
     * @param threads 线程数量
     * @param use_caller  是否将调用线程包含进去
     * @param name  调度器的名称
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    /**
     * @brief 析构函数
     */
    ~IOManager();

    /**
     * @brief 添加事件
     * @param fd  socket句柄
     * @param event 事件类型
     * @param cb  时间回调函数
     * @return  事件成功添加返回0，失败返回-1
     */
    int AddEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件
     * @param fd  socket句柄
     * @param event  事件类型
     * @return  返回是删除成功
     * @attention 不会触发事件
     */
    bool DelEvent(int fd, Event event);


    /**
     * @brief 取消事件
     * @param fd  socket句柄
     * @param event  事件类型
     * @return  如果事件存在则触发事件
     */
    bool CancelEvent(int fd, Event event);


    /**
     * @brief 取消所有事件
     * @param fd  socket句柄
     * @return
     */
    bool CancelAll(int fd);

    /**
     * @brief 返回当前的IOManager
     * @return
     */
    static IOManager* GetThis();


protected:

    void Tickle();
    bool Stopping();
    void Idle();
    void OnTimerInsertedAtFront();


    /**
     * @brief 重置socket句柄上下文的容器大小
     * @param size 容量大小
     */
    void ContextResize(size_t size);

    /**
     * @brief 判断是否可以停止
     * @param timeout  最近要触发的定时器事件间隔
     * @return  返回是否可以停止
     */
    bool Stopping(uint64_t& timeout);

private:

    // epoll 文件句柄
    int m_epfd = 0;
    // pipe 文件句柄
    int m_ticklefds[2];
    //当前等待执行的事件数量
    std::atomic<size_t > m_pendingEventCount{0};
    //IOManager的mutex
    ReaderWriterlatch m_mutex;
    // socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;


};


#endif //MYTINYWEBSERVER_IO_MANAGER_H
