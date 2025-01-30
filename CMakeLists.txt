cmake_minimum_required(VERSION 3.25)
project(testChat)

set(CMAKE_CXX_STANDARD 20)

add_executable(testChat testServer.cpp AQNThreadPool.h
        AQNThreadPool.cpp)
