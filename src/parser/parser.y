%skeleton "lalr1.cc"
%language "C++"
%define api.namespace {yy}
%define api.parser.class {parser}
%define api.value.type variant
%define api.token.constructor
%locations

%code requires {
    #include "AST.h"
    #include <memory>
    #include <vector>
    #include <string>
    
    // Forward declaration
    class SqlFrontend;
}

%lex-param { yy::location& yylloc }
%parse-param { std::unique_ptr<ASTNode>& result }
%parse-param { std::string& errorMsg }

%code {
    #include <FlexLexer.h>
    extern yyFlexLexer* current_lexer;
    
    static int yylex(yy::parser::semantic_type* yylval,
                     yy::parser::location_type* yylloc) {
        return current_lexer->yylex(yylval, yylloc);
    }
}

/* Объявление токенов */
%token END 0 "end of file"
%token SELECT INSERT UPDATE DELETE
%token CREATE DROP USE DATABASE TABLE
%token FROM WHERE SET VALUES INTO AS
%token AND OR BETWEEN LIKE NOT NULL_
%token INDEXED SUM COUNT AVG DEFAULT
%token EQ NE LE GE LT GT ASSIGN
%token SEMICOLON COMMA LPAREN RPAREN STAR
%token <int> INTEGER
%token <std::string> STRING IDENTIFIER
%token YYerror

/* Типы нетерминалов */
%type <std::unique_ptr<ASTNode>> query
%type <std::unique_ptr<ASTNode>> select_stmt insert_stmt update_stmt delete_stmt
%type <std::unique_ptr<ASTNode>> create_table_stmt drop_table_stmt
%type <std::unique_ptr<ASTNode>> create_database_stmt drop_database_stmt use_stmt
%type <std::unique_ptr<ASTNode>> condition expr literal column_ref
%type <std::unique_ptr<ASTNode>> and_condition or_condition comparison
%type <std::unique_ptr<ASTNode>> where_opt
%type <std::vector<std::string>> column_list select_columns
%type <std::vector<std::string>> opt_columns
%type <std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>> assignment_list
%type <std::vector<std::vector<std::unique_ptr<ASTNode>>>> values_list
%type <std::vector<std::unique_ptr<ASTNode>>> value_list

%start start

%%

start:
    query SEMICOLON
    {
        result = std::move($1);
    }
;

query:
    select_stmt      { $$ = std::move($1); }
    | insert_stmt    { $$ = std::move($1); }
    | update_stmt    { $$ = std::move($1); }
    | delete_stmt    { $$ = std::move($1); }
    | create_table_stmt { $$ = std::move($1); }
    | drop_table_stmt   { $$ = std::move($1); }
    | create_database_stmt { $$ = std::move($1); }
    | drop_database_stmt   { $$ = std::move($1); }
    | use_stmt           { $$ = std::move($1); }
;

/* SELECT запрос */
select_stmt:
    SELECT select_columns FROM IDENTIFIER where_opt
    {
        auto q = std::make_unique<SelectQuery>();
        q->columns = std::move($2);
        q->tableName = $4;
        if ($5) q->where = std::move($5);
        $$ = std::move(q);
    }
;

select_columns:
    STAR
    {
        $$ = std::vector<std::string>();
    }
    | column_list
    {
        $$ = std::move($1);
    }
;

column_list:
    IDENTIFIER
    {
        $$ = std::vector<std::string>{$1};
    }
    | column_list COMMA IDENTIFIER
    {
        $$ = std::move($1);
        $$.push_back($3);
    }
;

where_opt:
    /* empty */ { $$ = nullptr; }
    | WHERE condition { $$ = std::move($2); }
;

/* Условия с AND/OR и скобками */
condition:
    or_condition { $$ = std::move($1); }
;

or_condition:
    and_condition
    | or_condition OR and_condition
    {
        $$ = std::make_unique<OrOp>(std::move($1), std::move($3));
    }
;

and_condition:
    comparison
    | and_condition AND comparison
    {
        $$ = std::make_unique<AndOp>(std::move($1), std::move($3));
    }
;

comparison:
    expr EQ expr      { $$ = std::make_unique<BinaryOp>("==", std::move($1), std::move($3)); }
    | expr NE expr    { $$ = std::make_unique<BinaryOp>("!=", std::move($1), std::move($3)); }
    | expr LT expr    { $$ = std::make_unique<BinaryOp>("<",  std::move($1), std::move($3)); }
    | expr GT expr    { $$ = std::make_unique<BinaryOp>(">",  std::move($1), std::move($3)); }
    | expr LE expr    { $$ = std::make_unique<BinaryOp>("<=", std::move($1), std::move($3)); }
    | expr GE expr    { $$ = std::make_unique<BinaryOp>(">=", std::move($1), std::move($3)); }
    | expr BETWEEN expr AND expr
    {
        $$ = std::make_unique<BetweenOp>(std::move($1), std::move($3), std::move($5));
    }
    | expr LIKE expr
    {
        $$ = std::make_unique<LikeOp>(std::move($1), std::move($3));
    }
    | LPAREN condition RPAREN
    {
        $$ = std::move($2);
    }
;

expr:
    literal      { $$ = std::move($1); }
    | column_ref { $$ = std::move($1); }
;

literal:
    INTEGER      { $$ = std::make_unique<LiteralInt>($1); }
    | STRING     { $$ = std::make_unique<LiteralString>($1); }
    | NULL_      { $$ = std::make_unique<LiteralNull>(); }
;

column_ref:
    IDENTIFIER   { $$ = std::make_unique<ColumnRef>($1); }
;

/* INSERT запрос */
insert_stmt:
    INSERT INTO IDENTIFIER opt_columns VALUES values_list
    {
        auto q = std::make_unique<InsertQuery>();
        q->tableName = $3;
        q->columns = std::move($4);
        q->values = std::move($6);
        $$ = std::move(q);
    }
;

opt_columns:
    /* empty */ { $$ = std::vector<std::string>(); }
    | LPAREN column_list RPAREN { $$ = std::move($2); }
;

values_list:
    LPAREN value_list RPAREN
    {
        $$ = std::vector<std::vector<std::unique_ptr<ASTNode>>>{};
        $$.push_back(std::move($2));
    }
    | values_list COMMA LPAREN value_list RPAREN
    {
        $$ = std::move($1);
        $$.push_back(std::move($4));
    }
;

value_list:
    expr
    {
        $$ = std::vector<std::unique_ptr<ASTNode>>{};
        $$.push_back(std::move($1));
    }
    | value_list COMMA expr
    {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

/* UPDATE запрос */
update_stmt:
    UPDATE IDENTIFIER SET assignment_list where_opt
    {
        auto q = std::make_unique<UpdateQuery>();
        q->tableName = $2;
        q->assignments = std::move($4);
        if ($5) q->where = std::move($5);
        $$ = std::move(q);
    }
;

assignment_list:
    IDENTIFIER ASSIGN expr
    {
        $$ = std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>{};
        $$.emplace_back($1, std::move($3));
    }
    | assignment_list COMMA IDENTIFIER ASSIGN expr
    {
        $$ = std::move($1);
        $$.emplace_back($3, std::move($5));
    }
;

/* DELETE запрос */
delete_stmt:
    DELETE FROM IDENTIFIER where_opt
    {
        auto q = std::make_unique<DeleteQuery>();
        q->tableName = $3;
        if ($4) q->where = std::move($4);
        $$ = std::move(q);
    }
;

/* CREATE TABLE */
create_table_stmt:
    CREATE TABLE IDENTIFIER LPAREN column_definitions RPAREN
    {
        auto q = std::make_unique<CreateTableQuery>();
        q->tableName = $3;
        // column_definitions нужно реализовать
        $$ = std::move(q);
    }
;

column_definitions:
    // TODO: реализовать
;

/* DROP TABLE */
drop_table_stmt:
    DROP TABLE IDENTIFIER
    {
        auto q = std::make_unique<DropTableQuery>();
        q->tableName = $3;
        $$ = std::move(q);
    }
;

/* CREATE DATABASE */
create_database_stmt:
    CREATE DATABASE IDENTIFIER
    {
        auto q = std::make_unique<CreateDatabaseQuery>();
        q->dbName = $3;
        $$ = std::move(q);
    }
;

/* DROP DATABASE */
drop_database_stmt:
    DROP DATABASE IDENTIFIER
    {
        auto q = std::make_unique<DropDatabaseQuery>();
        q->dbName = $3;
        $$ = std::move(q);
    }
;

/* USE */
use_stmt:
    USE IDENTIFIER
    {
        auto q = std::make_unique<UseQuery>();
        q->dbName = $2;
        $$ = std::move(q);
    }
;

%%

void yy::parser::error(const location& loc, const std::string& msg) {
    errorMsg = "Parse error at line " + std::to_string(loc.begin.line) + 
               ", column " + std::to_string(loc.begin.column) + ": " + msg;
}