#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "AST.h"
#include "SqlParser.h"

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
        std::string current_query;
        int query_start = 0;
        while (std::getline(script, line)) {
            ++lineNum;
            // Убираем инлайн-комментарии
            if (auto pos = line.find("--"); pos != std::string::npos) line = line.substr(0, pos);
            // Тримминг справа
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), line.end());
            if (line.empty()) continue;

            if (current_query.empty()) query_start = lineNum;
            if (!current_query.empty()) current_query += " ";
            current_query += line;

            if (!current_query.empty() && current_query.back() == ';') {
                std::cout << "\n--- Query (lines " << query_start << "-" << lineNum << "): " << current_query << "\n";
                auto ast = frontend.parse(current_query); // передаём ВМЕСТЕ с ';'
                if (ast) std::cout << "AST: " << ast->toString() << std::endl;
                else std::cout << "Error: " << frontend.getLastError() << std::endl;
                current_query.clear();
            }
        }
        if (!current_query.empty()) std::cerr << "Line " << query_start << ": missing semicolon at EOF.\n";
    } else {
        std::cout << "Enter SQL queries (end with ';', empty line to exit):\n";
        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);
            if (line.empty()) break;
            auto ast = frontend.parse(line);
            if (ast) std::cout << "AST: " << ast->toString() << std::endl;
            else std::cout << "Error: " << frontend.getLastError() << std::endl;
        }
    }
    return 0;
}