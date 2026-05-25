cmake_minimum_required(VERSION 3.16)
project(DBMS_pain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

# ==================== Dependencies ====================
find_package(Boost REQUIRED)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

find_path(FLEXLEXER_INCLUDE_DIR FlexLexer.h
    PATHS /usr/include
    NO_DEFAULT_PATH
)

if(NOT FLEXLEXER_INCLUDE_DIR)
    message(FATAL_ERROR "FlexLexer.h not found!")
endif()

# ==================== Parser & Lexer (Clean) ====================

BISON_TARGET(SqlParser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
)

FLEX_TARGET(SqlLexer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/lexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
)

ADD_FLEX_BISON_DEPENDENCY(SqlLexer SqlParser)

# ==================== Main Executable ====================

add_executable(DBMS_pain
    src/main.cpp

    src/parser/AST.cpp
    src/parser/SqlParser.cpp

    src/storage/Serializer.cpp
    src/storage/RecordManager.cpp
    src/storage/PageManager.cpp

    src/catalog/SystemCatalog.cpp

    src/engine/Schema.cpp
    src/engine/Table.cpp
    src/engine/Database.cpp
    src/engine/Executor.cpp

    src/index/src/BStarPlusTree.cpp
    src/index/src/IndexManager.cpp
    src/index/src/pp_allocator.cpp

    # Generated
    ${BISON_SqlParser_OUTPUTS}
    ${FLEX_SqlLexer_OUTPUTS}
)

target_include_directories(DBMS_pain PRIVATE
    src
    src/index/include
    ${CMAKE_CURRENT_BINARY_DIR}        # Critical: for parser.h and lexer.cpp
    ${Boost_INCLUDE_DIRS}
    ${FLEXLEXER_INCLUDE_DIR}
)

target_compile_options(DBMS_pain PRIVATE 
    -Wall -Wextra -Wno-deprecated -Wno-unused-parameter
)

message(STATUS "✅ CMake configured successfully")