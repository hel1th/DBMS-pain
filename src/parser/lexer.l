%{
#undef YY_DECL
#define YY_DECL int SqlScanner::yylex(yy::parser::semantic_type* yylval, yy::parser::location_type* yyloc)

#include "parser/SqlScanner.h"
#include "parser.h"

void yyerror(const char* msg);
%}

%option c++
%option noyywrap
%option yylineno
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
("SELECT"|"select")     { return yy::parser::token::SELECT; }
("INSERT"|"insert")     { return yy::parser::token::INSERT; }
("UPDATE"|"update")     { return yy::parser::token::UPDATE; }
("DELETE"|"delete")     { return yy::parser::token::DELETE; }
("CREATE"|"create")     { return yy::parser::token::CREATE; }
("DROP"|"drop")         { return yy::parser::token::DROP; }
("USE"|"use")           { return yy::parser::token::USE; }
("DATABASE"|"database") { return yy::parser::token::DATABASE; }
("TABLE"|"table")       { return yy::parser::token::TABLE; }
("FROM"|"from")         { return yy::parser::token::FROM; }
("WHERE"|"where")       { return yy::parser::token::WHERE; }
("SET"|"set")           { return yy::parser::token::SET; }
("VALUE"|"value")       { return yy::parser::token::VALUE; }
("INTO"|"into")         { return yy::parser::token::INTO; }
("AS"|"as")             { return yy::parser::token::AS; }
("AND"|"and")           { return yy::parser::token::AND; }
("OR"|"or")             { return yy::parser::token::OR; }
("BETWEEN"|"between")   { return yy::parser::token::BETWEEN; }
("LIKE"|"like")         { return yy::parser::token::LIKE; }
("NOT"|"not")           { return yy::parser::token::NOT; }
("NULL"|"null")         { return yy::parser::token::NULL_; }
("INDEXED"|"indexed")   { return yy::parser::token::INDEXED; }
("SUM"|"sum")           { return yy::parser::token::SUM; }
("COUNT"|"count")       { return yy::parser::token::COUNT; }
("AVG"|"avg")           { return yy::parser::token::AVG; }
("DEFAULT"|"default")   { return yy::parser::token::DEFAULT; }
("REVERT"|"revert") { return yy::parser::token::REVERT; }
("int"|"INT")           { return yy::parser::token::INT_TYPE; }
("string"|"STRING")     { return yy::parser::token::STRING_TYPE; }

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
TIMESTAMP   [0-9]{4}\.[0-9]{2}\.[0-9]{2}-[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{6}
{TIMESTAMP}     {
                    yylval->emplace<std::string>(yytext);
                    return yy::parser::token::TIMESTAMP;
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
