cmake_minimum_required(VERSION 3.22)
project(MyTinyWebServer)



# 设置包含目录
include_directories(./src/include/)



# 链接可执行文件和库
add_executable(Server
        src/main.cpp
        src/config.cpp
        src/log.cpp
        src/sql_connection_pool.cpp
        src/lst_timer.cpp
        src/http_conn.cpp
        src/WebServer.cpp
        src/include/rwlatch.h
        src/fiber.cpp
        src/include/fiber.h
        src/scheduler.cpp
        src/include/scheduler.h
        src/include/io_manager.cpp
        src/include/io_manager.h
)
target_link_libraries(Server mysqlclient pthread)
add_compile_options(-g )