#pragma once
#include <istream>

#include "parser.h"

#ifndef yyFlexLexer
#define yyFlexLexer SqlScannerFlexLexer
#include <FlexLexer.h>
#endif

class SqlScanner : public SqlScannerFlexLexer {
public:
    explicit SqlScanner(std::istream& in) : SqlScannerFlexLexer(&in) {}
    using SqlScannerFlexLexer::yylex;
    int yylex(yy::parser::semantic_type* yylval, yy::parser::location_type* yyloc);
};
