cmake_minimum_required(VERSION 3.16)
project(BStarPlusTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Собираем все исходники из папки src
file(GLOB SOURCES "src/*.cpp")

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES} include/BStarPlusTree.h
    src/main.cpp)

# Указываем папку с заголовочными файлами проекта
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Находим Boost (для static_vector достаточно только заголовочных файлов)
find_package(Boost REQUIRED)

# Современный и безопасный способ линковки заголовочных файлов Boost
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::headers)