cmake_minimum_required(VERSION 3.15)

project(BehaviourTree
        DESCRIPTION "A small implementation of the Behaviour Tree design pattern"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall")

add_executable(BehaviourTree_test BehaviourTree_test.cpp)
target_link_libraries(BehaviourTree_test -lpthread)

enable_testing()
add_test(NAME BehaviourTree_example COMMAND BehaviourTree_test)
