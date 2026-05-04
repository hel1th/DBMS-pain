%{
#include "SqlScanner.h"
#include "parser.hpp"

#define YY_DECL int SqlScanner::yylex(yy::parser::semantic_type* yylval, yy::parser::location_type* yyloc)

void yyerror(const char* msg);
%}

%option c++
%option noyywrap
%option yylineno
%option outfile="lexer.cpp"
%option yyclass="SqlScanner"
%option prefix="SqlScanner"

/* Регулярные выражения */
DIGIT           [0-9]
LETTER          [a-zA-Z]
IDENT           {LETTER}({LETTER}|{DIGIT}|_)*
INTEGER         {DIGIT}+
STRING          \"([^\\\"]|\\.)*\"
WHITESPACE      [ \t\r\n]+

%%

 /* Ключевые слова (регистр неважен) */
(?i:SELECT)     { return yy::parser::token::SELECT; }
(?i:INSERT)     { return yy::parser::token::INSERT; }
(?i:UPDATE)     { return yy::parser::token::UPDATE; }
(?i:DELETE)     { return yy::parser::token::DELETE; }
(?i:CREATE)     { return yy::parser::token::CREATE; }
(?i:DROP)       { return yy::parser::token::DROP; }
(?i:USE)        { return yy::parser::token::USE; }
(?i:DATABASE)   { return yy::parser::token::DATABASE; }
(?i:TABLE)      { return yy::parser::token::TABLE; }
(?i:FROM)       { return yy::parser::token::FROM; }
(?i:WHERE)      { return yy::parser::token::WHERE; }
(?i:SET)        { return yy::parser::token::SET; }
(?i:VALUES)     { return yy::parser::token::VALUES; }
(?i:INTO)       { return yy::parser::token::INTO; }
(?i:AS)         { return yy::parser::token::AS; }
(?i:AND)        { return yy::parser::token::AND; }
(?i:OR)         { return yy::parser::token::OR; }
(?i:BETWEEN)    { return yy::parser::token::BETWEEN; }
(?i:LIKE)       { return yy::parser::token::LIKE; }
(?i:NOT)        { return yy::parser::token::NOT; }
(?i:NULL)       { return yy::parser::token::NULL_; }
(?i:INDEXED)    { return yy::parser::token::INDEXED; }
(?i:SUM)        { return yy::parser::token::SUM; }
(?i:COUNT)      { return yy::parser::token::COUNT; }
(?i:AVG)        { return yy::parser::token::AVG; }
(?i:DEFAULT)    { return yy::parser::token::DEFAULT; }
"int"           { return yy::parser::token::INT_TYPE; }
"string"        { return yy::parser::token::STRING_TYPE; }

 /* Операторы */
"=="            { return yy::parser::token::EQ; }
"!="            { return yy::parser::token::NE; }
"<="            { return yy::parser::token::LE; }
">="            { return yy::parser::token::GE; }
"<"             { return yy::parser::token::LT; }
">"             { return yy::parser::token::GT; }
"="             { return yy::parser::token::ASSIGN; }

 /* Разделители */
";"             { return yy::parser::token::SEMICOLON; }
","             { return yy::parser::token::COMMA; }
"("             { return yy::parser::token::LPAREN; }
")"             { return yy::parser::token::RPAREN; }
"*"             { return yy::parser::token::STAR; }

 /* Литералы */
{INTEGER}       { 
                    yylval->emplace<int>(std::stoi(yytext));
                    return yy::parser::token::INTEGER; 
                }
{STRING}        { 
                    std::string s(yytext + 1);
                    s.pop_back();  // убираем последнюю кавычку
                    // Обработка экранирования
                    size_t pos = 0;
                    while ((pos = s.find("\\\"", pos)) != std::string::npos) {
                        s.replace(pos, 2, "\"");
                        pos++;
                    }
                    yylval->emplace<std::string>(s);
                    return yy::parser::token::STRING; 
                }
{IDENT}         { 
                    yylval->emplace<std::string>(yytext);
                    return yy::parser::token::IDENTIFIER; 
                }

 /* Пропуск пробелов и переводов строк */
{WHITESPACE}    { /* ignore */ }

 /* Обработка ошибок */
.               { 
                    std::string msg = "Unexpected character: ";
                    msg += yytext;
                    yyloc->initialize();
                    yyloc->lines(yylineno);
                    yyerror(msg.c_str());
                    return yy::parser::token::YYerror;
                }

%%