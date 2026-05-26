cmake_minimum_required(VERSION 3.16)
project(DBMS_pain LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

# ==================== Dependencies ====================
find_package(Boost CONFIG REQUIRED)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

find_path(FLEXLEXER_INCLUDE_DIR FlexLexer.h
    PATHS
        /usr/include
        /usr/local/include
        /usr/local/opt/flex/include
        /opt/homebrew/opt/flex/include
        /usr/include/flex
    NO_DEFAULT_PATH
    DOC "Directory containing FlexLexer.h"
)
if(NOT FLEXLEXER_INCLUDE_DIR)
    message(FATAL_ERROR "FlexLexer.h not found! Install flex + libfl-dev")
else()
    message(STATUS "Found FlexLexer.h in ${FLEXLEXER_INCLUDE_DIR}")
endif()

# ==================== Parser & Lexer ====================
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

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/parser.h
    PROPERTIES GENERATED TRUE
)

# ==================== Main Executable ====================
add_executable(DBMS_pain
    src/tests/TestUndoLog.cpp
    src/tests/TestParser.cpp

    src/main.cpp
    src/parser/AST.cpp
    src/parser/SqlParser.cpp
    src/storage/Serializer.cpp
    src/storage/RecordManager.cpp
    src/storage/PageManager.cpp
    src/storage/UndoLogManager.cpp
    src/catalog/SystemCatalog.cpp
    src/engine/Schema.cpp
    src/engine/Table.cpp
    src/engine/Database.cpp
    src/engine/Executor.cpp
    src/index/src/BStarPlusTree.cpp
    src/index/src/IndexManager.cpp
    src/index/src/pp_allocator.cpp
    ${BISON_SqlParser_OUTPUTS}
    ${FLEX_SqlLexer_OUTPUTS}
)


target_include_directories(DBMS_pain PRIVATE
    src
    src/index/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${FLEXLEXER_INCLUDE_DIR}
)
target_compile_options(DBMS_pain PRIVATE
    -Wall -Wextra -Wno-deprecated -Wno-unused-parameter
)
message(STATUS "======= CMake configured successfully =======")