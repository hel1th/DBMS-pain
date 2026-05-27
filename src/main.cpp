#include <iostream>
#include "tests/TestModules.h"

int main(int argc, char** argv) {
    if (argc == 2) {
        testUndo();
        const char* args[] = {"dbms", "src/tests/test_queries.sql"};
        testParser(2, const_cast<char**>(args));
    }
    TestExecutor();
    std::cout << "abababa" << std::endl;
}
