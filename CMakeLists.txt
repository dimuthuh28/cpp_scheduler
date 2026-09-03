cmake_minimum_required(VERSION 3.14)
project(cpp_task_scheduler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

add_library(scheduler
    src/Logger.cpp
    src/PrintTask.cpp
    src/CalculationTask.cpp
    src/TaskScheduler.cpp
)
target_include_directories(scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(scheduler PUBLIC Threads::Threads)

add_executable(demo examples/main.cpp)
target_link_libraries(demo PRIVATE scheduler)

add_executable(benchmark examples/benchmark.cpp)
target_link_libraries(benchmark PRIVATE scheduler)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()

    add_executable(scheduler_tests tests/TaskSchedulerTest.cpp)
    target_link_libraries(scheduler_tests PRIVATE scheduler GTest::gtest GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(scheduler_tests)
endif()
