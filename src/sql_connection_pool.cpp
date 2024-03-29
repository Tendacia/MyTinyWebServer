//
// Created by tendacia on 3/1/24.
//

#include <pthread.h>

#include "sql_connection_pool.h"


//当有请求时，从数据库连接池中返回一个可用连接，更新和使用空闲连接数
MYSQL *connection_pool::GetConnection() {
    MYSQL * con = NULL;

    //@Todo 互斥访问临界资源
    lock.lock();
    if(0 == connList.size())
    {
        return NULL;
    }
    lock.unlock();

    reserve.wait();

    lock.lock();

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return con;
}

//释放当前连接
bool connection_pool::ReleaseConection(MYSQL *con) {
    if(NULL == con)
    {
        return false;
    }

    lock.lock();

    connList.push_back(con);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    reserve.post();
    return true;
}

//当前空闲的连接数
//@Todo 是不是需要线程安全，这函数的目的是啥？
int connection_pool::GetFreeConn() {
    int tmp = 0;

    lock.lock();
    tmp = m_FreeConn;
    lock.unlock();

    return tmp;
}

//销毁数据库连接池
void connection_pool::DestroyPool() {
    lock.lock();
    if(connList.size() > 0)
    {
        for(auto it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }

    lock.unlock();
}

connection_pool *connection_pool::GetInstance() {
    static connection_pool connPool;
    return &connPool;
}

//构造初始化
void connection_pool::init(std::string url, std::string User, std::string PassWord, std::string DatabaseName, int Port,
                           int MaxConn, int close_log) {
        m_url = url;
        m_Port = Port;
        m_User = User;
        m_PassWord = PassWord;
        m_DatabaseName = DatabaseName;
        m_close_log = close_log;


        for(int i = 0; i <MaxConn; ++i)
        {
            MYSQL *con = NULL;
            con = mysql_init(con);

            if(con == NULL)
            {
                LOG_DEBUG("MySQL Error");
                exit(1);
            }
            con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(),
                                     DatabaseName.c_str(), Port, NULL, 0);

            if(con == NULL)
            {
                LOG_ERROR("MySQL Error");
                exit(1);
            }
            connList.push_back(con);
            ++m_FreeConn;
        }

        reserve = sem(m_FreeConn);
        m_MaxConn = m_FreeConn;
}

connection_pool::connection_pool() {
    m_CurConn = 0;
    m_FreeConn = 0;
}

connection_pool::~connection_pool() {
    DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool) {
    *SQL = connPool->GetConnection();

    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
    poolRAII->ReleaseConection(conRAII);
}
