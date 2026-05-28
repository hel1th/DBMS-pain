#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Подключение модулей согласно структуре проекта
#include "engine/Executor.h"
#include "parser/AST.h"
#include "parser/SqlParser.h"

using namespace std;

// Функция обработки текстового буфера запроса
void processQueryBuffer(Executor& executor, SqlParser& parser, const string& queryText) {
    // Пропускаем пустые запросы (пробелы, переносы строк)
    if (queryText.find_first_not_of(" \t\n\r") == string::npos) {
        return;
    }

    try {
        // Передаем запрос в парсер и получаем владение через unique_ptr
        std::unique_ptr<ASTNode> astRoot = parser.parse(queryText);

        if (!astRoot) {
            // Выводим ошибку синтаксиса, полученную из Flex/Bison через getLastError()
            cerr << "Syntax Error: " << parser.getLastError() << endl;
            return;
        }

        // Выполняем запрос. Метод execute принимает сырой указатель ASTNode*
        QueryResult result = executor.execute(astRoot.get());

        // Проверяем статус выполнения (выбросит false при семантических ошибках)
        if (!result.ok) {
            cerr << "Execution Error: " << result.error << endl;
        } else {
            if (!result.rows.empty()) {
                // Если это SELECT — выводим результат в формате JSON
                cout << executor.toJSON(result.rows) << endl;
            } else {
                // Для DML/DDL команд без возвращаемого набора строк
                cout << "Query OK, " << result.affected << " rows affected." << endl;
            }
        }
        // Память, выделенная под AST-дерево, автоматически освобождается здесь
        // при выходе astRoot из области видимости.

    } catch (const exception& e) {
        // Защита от падения всей СУБД при непредвиденных исключениях
        cerr << "Runtime Exception: " << e.what() << endl;
    }
}

// 1. Интерактивный режим
void runInteractiveMode(Executor& executor, SqlParser& parser) {
    string queryBuffer;
    string line;

    cout << "DBMS Interactive Mode started. All commands must end with ';'.\n";
    cout << "Type 'exit' or 'quit' to close the session.\n";
    cout << "dbms> ";

    while (getline(cin, line)) {
        if (line == "exit" || line == "quit" || line == "exit;" || line == "quit;") {
            break;
        }

        queryBuffer += line + "\n";

        // Выделяем запросы по точке с запятой (поддержка многострочного ввода)
        size_t semiPos;
        while ((semiPos = queryBuffer.find(';')) != string::npos) {
            string singleQuery = queryBuffer.substr(0, semiPos + 1);
            processQueryBuffer(executor, parser, singleQuery);
            queryBuffer = queryBuffer.substr(semiPos + 1);
        }

        cout << "dbms> ";
    }
}

// 2. Пакетный режим
void runBatchMode(Executor& executor, SqlParser& parser, const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Fatal Error: Could not open script file '" << filename << "'" << endl;
        return;
    }

    string queryBuffer;
    string line;

    while (getline(file, line)) {
        queryBuffer += line + "\n";

        size_t semiPos;
        while ((semiPos = queryBuffer.find(';')) != string::npos) {
            string singleQuery = queryBuffer.substr(0, semiPos + 1);
            processQueryBuffer(executor, parser, singleQuery);
            queryBuffer = queryBuffer.substr(semiPos + 1);
        }
    }

    // Дорабатываем остаток буфера, если файл не заканчивался на ';'
    if (!queryBuffer.empty() && queryBuffer.find_first_not_of(" \t\n\r") != string::npos) {
        processQueryBuffer(executor, parser, queryBuffer);
    }
}

int main(int argc, char** argv) {
    // Инстанцируем один раз, чтобы сохранить состояние сессии (активную БД, кэш таблиц)
    Executor executor;
    SqlParser parser;

    if (argc == 1) {
        // Без аргументов -> Интерактивный режим
        runInteractiveMode(executor, parser);
    } else if (argc == 2) {
        // Один аргумент -> Пакетный режим (чтение файла)
        runBatchMode(executor, parser, argv[1]);
    } else {
        cerr << "Usage:\n"
             << "  Interactive mode: " << argv[0] << "\n"
             << "  Batch mode:       " << argv[0] << " <script_file.sql>" << endl;
        return 1;
    }

    return 0;
}
