//
// Created by tendacia on 3/1/24.
//

/**
 * @brief 管理mysql的连接池
 */
#ifndef MYTINYWEBSERVER_SQL_CONNECTION_POOL_H
#define MYTINYWEBSERVER_SQL_CONNECTION_POOL_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>


#include "locker.h"
#include "log.h"





class connection_pool {

public:
    MYSQL *GetConnection(); //获取数库据连接
    bool ReleaseConection(MYSQL* con); //释放连接
    int GetFreeConn();  //获取连接
    void DestroyPool(); //销毁所有连接

    //单例
    static  connection_pool* GetInstance();

    void init(std::string url, std::string User, std::string PassWord, std::string DatabaseName,
              int Port, int MaxConn, int close_log);

private:
    connection_pool();
    ~connection_pool();

    int m_MaxConn; //最大连接数
    int m_CurConn; //当前已使用连接数
    int m_FreeConn; //当前空闲连接数
    locker lock;
    std::list<MYSQL *> connList; //连接池
    sem reserve;


public:
    std::string m_url; //主体地址
    std::string m_Port; //数据库端口号
    std::string m_User; //登录数据库名
    std::string m_PassWord; //登录数据库密码
    std::string m_DatabaseName; //使用数据库名
    int m_close_log; //日志开关
};


class connectionRAII
{
public:
    connectionRAII(MYSQL** conn, connection_pool* connPool);
    ~connectionRAII();

private:
    MYSQL * conRAII;
    connection_pool* poolRAII;
};


#endif //MYTINYWEBSERVER_SQL_CONNECTION_POOL_H
