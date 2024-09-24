cmake_minimum_required(VERSION 3.22)
project(Archiver C)

set(CMAKE_C_STANDARD 11)

add_executable(Archiver main.c
        archiver.c
        archiver.h)
