#include <fstream>
#include <iostream>
#include <string>
#include "engine/DBMS_Engine.h"

using namespace std;

void runBatchMode(dbms::DBMSEngine& dbmsEngine, const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: Cannot open script file " << filepath << endl;
        return;
    }

    string line;
    string queryBuffer;

    while (getline(file, line)) {
        if (line.rfind("--", 0) == 0) {
            continue;
        }

        queryBuffer += line + "\n";

        if (line.find(';') != string::npos) {
            // Передаем session_id как "batch", чтобы в логах отличать файлы от консоли
            dbmsEngine.processQueryBuffer(queryBuffer, "batch");
            queryBuffer.clear();
        }
    }

    // Обрабатываем остаток, если в конце файла забыли поставить ';'
    if (!queryBuffer.empty() && queryBuffer.find_first_not_of(" \t\n\r") != string::npos) {
        dbmsEngine.processQueryBuffer(queryBuffer, "batch");
    }
}

int main(int argc, char** argv) {
    dbms::DBMSEngine dbmsEngine;

    if (argc == 1) {
        // Интерактивный режим
        std::string line;
        std::cout << "DBMS_pain> ";
        while (std::getline(std::cin, line)) {
            if (line == "exit;")
                break;


            dbmsEngine.processQueryBuffer(line, "interactive");

            std::cout << "DBMS_pain> ";
        }
    } else if (argc == 2) {
        // Пакетный режим (make run)
        runBatchMode(dbmsEngine, argv[1]);
    } else {
        cerr << "Usage:\n"
             << "  Interactive mode: " << argv[0] << "\n"
             << "  Batch mode:       " << argv[0] << " <script_file.sql>" << endl;
        return 1;
    }

    return 0;
}
