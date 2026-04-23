cmake_minimum_required(VERSION 3.10)
project(bsptree_lib)

add_library(bsptree
    BStarPlusTree.cpp
    IndexManager.cpp
)

target_include_directories(bsptree PUBLIC .)

target_link_libraries(bsptree PUBLIC sys_prog_assctv_cntnr)


// тут нет рабочей сборки, а должна быть