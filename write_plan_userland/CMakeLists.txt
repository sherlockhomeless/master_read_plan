cmake_minimum_required(VERSION 3.19)
project(write_plan_userland C)

set(CMAKE_C_STANDARD 11)

add_executable(write_plan_userland
        write_plan.c)
