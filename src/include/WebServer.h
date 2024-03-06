//
// Created by tendacia on 3/2/24.
//

#ifndef MYTINYWEBSERVER_WEBSERVER_H
#define MYTINYWEBSERVER_WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool.h"
#include "http_conn.h"

const int MAX_FD = 65535; //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TiMESLOT = 3; //最小超时单位






class WebServer {

public:
    WebServer();
    ~WebServer();

    void init(int port,std::string user, std::string password, std::string database_name, int log_write, int opt_liner, int trig_mode,
         int sql_num, int thread_num, int close_log, int actor_model);

    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);


public:
    //基础
    int m_port;
    char *m_root;
    int m_log_write;
    int m_close_log;
    int m_actor_model;

    int m_pipefd[2];
    int m_epollfd;
    http_conn *users;

    //数据库相关
    connection_pool *m_conn_pool;
    std::string m_user;  //登录数据库用户名
    std::string m_password; //登录数据库密码
    std::string m_database_name; //使用数据库名
    int m_sql_num;

    //线程相关
    threadPool<http_conn> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;

    //定时器相关
    client_data *user_timer;
    Utls utils;





};


#endif //MYTINYWEBSERVER_WEBSERVER_H
