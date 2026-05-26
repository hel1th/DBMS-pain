#include "SqlParser.h"
#include "AST.h"
#include "parser.h"
#include "SqlScanner.h"
#include <sstream>
#include <memory>

class SqlParser::Impl {
public:
    std::string lastError;

    std::unique_ptr<ASTNode> parse(const std::string& query) {
        std::stringstream ss(query);
        SqlScanner scanner(ss);

        std::unique_ptr<ASTNode> result;
        std::string errorMsg;

        yy::parser parser(result, errorMsg, scanner);
        int parseResult = parser.parse();
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

void yyerror(const char* msg) {
    std::cerr << "Parse error: " << msg << std::endl;
}