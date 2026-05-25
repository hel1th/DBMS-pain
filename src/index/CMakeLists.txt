cmake_minimum_required(VERSION 3.16)
project(BStarPlusTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Создаем исполняемый файл
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/tests.cpp
    src/IndexManager.cpp
    ../storage/RecordManager.cpp
    ../utils/Error.h
)

# Указываем ВСЕ необходимые директории с заголовками
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # Корень проекта (где лежит CMakeLists.txt)
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # Для BStarPlusTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils    # Для Error.h, Value.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../storage  # Для RecordManager.h, PageManager.h
)

# Находим Boost
find_package(Boost REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::headers)