//
// Created by tendacia on 2/29/24.
//
#include <string>
#include <thread>

#include "include/block_queue.h"
#include "include/config.h"
#include "include/log.h"
#include "include/log.h"
#include "include/threadpool.h"


int main(int argc, char ** argv)
{
    std::string user = "root";
    std::string passwd = "123456";
    std::string  database_name = "yourdb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config.PORT, user, passwd, database_name, config.LOGWrite,
                config.OPT_LINGER, config.TRIGMode, config.sql_num, config.thread_num,
                config.close_log, config.actor_model);

    //日志
    server.log_write();

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //触发模式
    server.trig_mode();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    exit(EXIT_SUCCESS);


}

