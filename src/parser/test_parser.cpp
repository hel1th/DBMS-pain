// tests/test_parser.cpp
#include <fstream>
#include <iostream>
#include <string>
#include "parser/AST.h"
#include "parser/SqlParser.h"


int main(int argc, char* argv[]) {
    SqlParser frontend;

    if (argc >= 2) {
        std::ifstream script(argv[1]);
        if (!script.is_open()) {
            std::cerr << "Cannot open file: " << argv[1] << std::endl;
            return 1;
        }
        std::string line;
        int lineNum = 0;
        while (std::getline(script, line)) {
            ++lineNum;
            // Пропускаем пустые строки и комментарии (-- начинается комментарий)
            if (line.empty() || (line.size() >= 2 && line.substr(0, 2) == "--"))
                continue;
            // Запрос может быть многострочным? Упростим: каждая строка – отдельный запрос,
            // оканчивающийся на ';'

            // TODO поправить проверку на ';' в конце строки, сейчас она неверная
            // Отрезать все пробельные символы в конце строки и символы переноса строки \r \n
            if (line.back() == ';') {
                std::cerr << "Line " << lineNum << ": missing semicolon, skipping: " << line
                          << std::endl;
                continue;
            }
            std::cout << "\n--- Query: " << line << "\n";
            auto ast = frontend.parse(line);
            if (ast) {
                std::cout << "AST: " << ast->toString() << std::endl;
            } else {
                std::cout << "Error: " << frontend.getLastError() << std::endl;
            }
        }
    } else {
        std::cout << "Enter SQL queries (end with ';', empty line to exit):\n";
        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);
            if (line.empty())
                break;
            if (line.back() != ';') {
                std::cout << "Missing semicolon, ignoring.\n";
                continue;
            }
            auto ast = frontend.parse(line);
            if (ast) {
                std::cout << "AST: " << ast->toString() << std::endl;
            } else {
                std::cout << "Error: " << frontend.getLastError() << std::endl;
            }
        }
    }
    return 0;
}
