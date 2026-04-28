#include "SqlParser.h"
#include "AST.h"
#include "parser.hpp"
#include <FlexLexer.h>
#include <sstream>

// Глобальный указатель на текущий лексер (необходим для yylex)
yyFlexLexer* current_lexer = nullptr;

// Функция, вызываемая Bison'ом
static int yylex(yy::parser::semantic_type* yylval, yy::parser::location_type* yyloc) {
    if (!current_lexer) return 0;
    return current_lexer->yylex(yylval, yyloc);
}

// Обработчик ошибок (будет вызван из Bison). Проверить на 2 определения!!!
namespace yy {
    void parser::error(const location& loc, const std::string& msg) {
    }
}

// Реализация PIMPL
class SqlParser::Impl {
public:
    std::string lastError;

    std::unique_ptr<ASTNode> parse(const std::string& query) {
        std::stringstream ss(query);
        yyFlexLexer lexer(&ss);
        current_lexer = &lexer;

        std::unique_ptr<ASTNode> result;
        std::string errorMsg;

        yy::parser parser(result, errorMsg);
        int parseResult = parser.parse();

        current_lexer = nullptr;

        if (parseResult != 0 || !result) {
            lastError = errorMsg;
            return nullptr;
        }
        return result;
    }

    bool validate(const std::string& query) {
        auto ast = parse(query);
        return ast != nullptr;
    }

    std::string getLastError() const {
        return lastError;
    }
};

SqlParser::SqlParser() : pImpl(std::make_unique<Impl>()) {}
SqlParser::~SqlParser() = default;

std::unique_ptr<ASTNode> SqlParser::parse(const std::string& query) {
    return pImpl->parse(query);
}

bool SqlParser::validate(const std::string& query) {
    return pImpl->validate(query);
}

std::string SqlParser::getLastError() const {
    return pImpl->getLastError();
}