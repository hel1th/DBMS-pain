add_executable(test_executor
        test_executor.cpp

        ../src/parser/AST.cpp
        ../src/parser/SqlParser.cpp
        ../src/storage/PageManager.cpp
        ../src/storage/Serializer.cpp
        ../src/storage/RecordManager.cpp
        ../src/catalog/SystemCatalog.cpp
        ../src/engine/Schema.cpp
        ../src/engine/Table.cpp
        ../src/engine/Database.cpp
        ../src/engine/Executor.cpp
        ../src/index/src/BStarPlusTree.cpp
        ../src/index/src/IndexManager.cpp
        ../src/index/src/pp_allocator.cpp

        ${BISON_SqlParser_OUTPUTS}
        ${FLEX_SqlLexer_OUTPUTS}
)

target_include_directories(test_executor PRIVATE
        ../src
        ../src/index/include
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${Boost_INCLUDE_DIRS}
)