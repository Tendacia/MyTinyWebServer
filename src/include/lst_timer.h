//
// Created by tendacia on 3/1/24.
//

/**
 * @brief 使用定时器来处理长时间不活动的连接
 */

#ifndef MYTINYWEBSERVER_LST_TIMER_H
#define MYTINYWEBSERVER_LST_TIMER_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <error.h>
#include <sys/wait.h>
#include <sys/uio.h>


#include <time.h>

#include "log.h"


class util_timer;


struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer* timer;
};

class util_timer
{
public:
    util_timer(): prev(NULL), next(NULL){}

public:
    time_t expire;

    void (*cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer * next;
};


class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();


    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_tiemer(util_timer *timer);
    void tick();


private:
    void add_timer(util_timer *timer, util_timer *lst_head);

    util_timer *head;
    util_timer *tail;
};


class Utls
{
public:
    Utls(){}
    ~Utls(){}

    void init(int timeslot);

    //对文件描述符设置为非阻塞
    int setNonBlocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handlder(int sig);

    //设置信号处理函数
    void addsig(int sig, void (handler)(int), bool restart = true);

    //定时处理任务，重新定时以不端触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char * info);

public:
    static int *u_pipfd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);






#endif //MYTINYWEBSERVER_LST_TIMER_H
