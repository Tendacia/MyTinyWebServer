//
// Created by tendacia on 2/29/24.
//
/**
 * @brief 根据命令行参数对服务器进行配置
 */

#ifndef MYTINYWEBSERVER_CONFIG_H
#define MYTINYWEBSERVER_CONFIG_H


#include "WebServer.h"

class Config {
public:
    Config();
    ~Config();

    void parse_arg(int argc, char** argv);

    // port number
    int PORT;

    // log method
    int LOGWrite;

    //Trigger mode
    int TRIGMode;

    //Listenfd trigger mode
    int LISTENTrigMode;

    //Connfd trigger mode
    int CONNTrigMode;

    //Close Mode
    int OPT_LINGER;

    //num of sql connection
    int sql_num;

    //num of threads in threads pool
    int thread_num;

    //close log
    int close_log;

    //Concurrency model
    int actor_model;

};


#endif //MYTINYWEBSERVER_CONFIG_H
