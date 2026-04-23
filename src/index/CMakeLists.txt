cmake_minimum_required(VERSION 3.16)
project(BStarPlusTree LANGUAGES CXX)

set(CMAKE_CXX_COMPILER "g++")  # или "cl.exe" для MSVC
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исходники
file(GLOB SOURCES "src/*.cpp")

# Сначала создаём цель (исполняемый файл)
add_executable(${PROJECT_NAME} ${SOURCES})

# Теперь можно добавлять include-директории для этой цели
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Поиск Boost (static_vector header-only)
find_package(Boost REQUIRED)
if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
endif()