cmake_minimum_required(VERSION 3.25)
project(testServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(testServer
        testServer.cpp
        AQNThreadPool.h
        AQNThreadPool.cpp
        MessageDispatcher.h
        MessageDispatcher.cpp)
