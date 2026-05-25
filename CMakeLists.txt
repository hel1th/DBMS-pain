cmake_minimum_required(VERSION 3.16)
project(DBMS_pain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#  Boost (header-only, needed by B*+-tree) 
find_package(Boost REQUIRED)

#  Flex / Bison 
find_package(BISON REQUIRED)
find_package(FLEX  REQUIRED)

BISON_TARGET(SqlParser
        ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
        ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
)
FLEX_TARGET(SqlLexer
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l
        ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
)
ADD_FLEX_BISON_DEPENDENCY(SqlLexer SqlParser)

#  Sources 
add_executable(DBMS_pain
        src/main.cpp

        # Parser (hand-written)
        src/parser/AST.cpp
        src/parser/AST.h
        src/parser/SqlParser.cpp
        src/parser/SqlParser.h

        # Storage layer
        src/storage/PageManager.cpp
        src/storage/PageManager.h
        src/storage/Serializer.cpp
        src/storage/Serializer.h
        src/storage/RecordManager.cpp
        src/storage/RecordManager.h

        # Catalog
        src/catalog/SystemCatalog.cpp
        src/catalog/SystemCatalog.h

        # Engine
        src/engine/Schema.cpp
        src/engine/Schema.h
        src/engine/Table.cpp
        src/engine/Table.h
        src/engine/Database.cpp
        src/engine/Database.h

        # Index (B*+-tree)
        src/index/include/associativeContainer.h
        src/index/include/BStarPlusTree.h
        src/index/include/IndexManager.h
        src/index/include/pp_allocator.h
        src/index/src/BStarPlusTree.cpp
        src/index/src/IndexManager.cpp
        src/index/src/pp_allocator.cpp

        # Generated lexer / parser
        ${BISON_SqlParser_OUTPUTS}
        ${FLEX_SqlLexer_OUTPUTS}

        # Utility headers (header-only, listed for IDE visibility)
        src/utils/Value.h
        src/utils/Error.h
)

target_include_directories(DBMS_pain PRIVATE
        src                                   # project-wide includes
        src/index/include                     # B*+-tree headers
        ${CMAKE_CURRENT_BINARY_DIR}           # generated parser/lexer headers
        ${Boost_INCLUDE_DIRS}                 # Boost (static_vector, etc.)
)