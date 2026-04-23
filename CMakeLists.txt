cmake_minimum_required(VERSION 4.1)
project(DBMS_pain)

set(CMAKE_CXX_STANDARD 20)

add_executable(DBMS_pain
        src/main.cpp
        src/parser/AST.cpp
        src/parser/AST.h
        src/parser/SqlParser.cpp
        src/parser/SqlParser.h
        src/index/BPlusTree.cpp
        src/index/BPlusTree.h
        src/index/IndexManager.cpp
        src/index/IndexManager.h
        src/storage/PageManager.cpp
        src/storage/PageManager.h
        src/storage/Serializer.cpp
        src/storage/Serializer.h
        src/catalog/SystemCatalog.cpp
        src/catalog/SystemCatalog.h
        src/utils/Value.h
        src/utils/Error.h
        src/storage/RecordManager.cpp
        src/storage/RecordManager.h
        src/engine/Schema.cpp
        src/engine/Schema.h
        src/engine/Table.cpp
        src/engine/Table.h
        src/engine/Database.cpp
        src/engine/Database.h
)

target_include_directories(DBMS_pain PRIVATE src)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(SqlParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(SqlLexer lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)

ADD_FLEX_BISON_DEPENDENCY(SqlLexer SqlParser)
