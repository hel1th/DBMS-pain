%skeleton "lalr1.cc"
%language "C++"
%define api.namespace {yy}
%define api.parser.class {parser}
%define api.value.type variant
%locations
%param { std::unique_ptr<ASTNode>& result }
%param { std::string& errorMsg }
%parse-param { SqlScanner& scanner }
%lex-param { SqlScanner& scanner }

%code requires {
    #include "parser/AST.h"
    #include <memory>
    #include <vector>
    #include <string>
    #include "utils/Value.h"
    class SqlScanner;
}

%code {
    #include "parser/SqlScanner.h"
        static int yylex(yy::parser::semantic_type* yylval,
                 yy::parser::location_type* yyloc,
                 std::unique_ptr<ASTNode>& /*result*/,
                 std::string& /*errorMsg*/,
                 SqlScanner& scanner) {
        return scanner.yylex(yylval, yyloc);
    }
}

/* Токены */
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
%token INT_TYPE STRING_TYPE

/* Типы нетерминалов */
%type <std::unique_ptr<ASTNode>> query
%type <std::unique_ptr<ASTNode>> select_stmt insert_stmt update_stmt delete_stmt
%type <std::unique_ptr<ASTNode>> create_table_stmt drop_table_stmt
%type <std::unique_ptr<ASTNode>> create_database_stmt drop_database_stmt use_stmt
%type <std::unique_ptr<ASTNode>> condition expr literal column_ref
%type <std::unique_ptr<ASTNode>> and_condition or_condition comparison
%type <std::unique_ptr<ASTNode>> where_opt

%type <SelectColumn> select_item
%type <std::vector<SelectColumn>> select_columns
%type <AggregateExpr> aggregate_expr
%type <std::string> column_name_or_star
%type <std::vector<std::string>> column_list opt_columns
%type <std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>> assignment_list
%type <std::vector<std::vector<std::unique_ptr<ASTNode>>>> values_list
%type <std::vector<std::unique_ptr<ASTNode>>> value_list
%type <std::vector<ColumnSpec>> column_definitions
%type <ColumnSpec> column_definition
%type <ColType> type_spec
%type <bool> not_null_opt indexed_opt
%type <Value> default_opt

%start start

%%

start:
    query SEMICOLON { result = std::move($1); }
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

/* SELECT */
select_stmt:
    SELECT select_columns FROM IDENTIFIER where_opt
    {
        auto q = std::make_unique<SelectQuery>();
        q->columns = std::move($2);
        q->tableName = $4;
        q->star = false;
        if ($5) q->where = std::move($5);
        $$ = std::move(q);
    }
    | SELECT STAR FROM IDENTIFIER where_opt
    {
        auto q = std::make_unique<SelectQuery>();
        q->star = true;
        q->tableName = $4;
        if ($5) q->where = std::move($5);
        $$ = std::move(q);
    }
;

select_columns:
    select_item
    {
        $$.clear();
        $$.push_back(std::move($1));
    }
    | select_columns COMMA select_item
    {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

select_item:
    IDENTIFIER
    {
        SelectColumn col;
        col.name = $1;
        col.alias = "";
        $$ = std::move(col);
    }
    | IDENTIFIER AS IDENTIFIER
    {
        SelectColumn col;
        col.name = $1;
        col.alias = $3;
        $$ = std::move(col);
    }
    | aggregate_expr
    {
        // Агрегаты хранятся отдельно, но для простоты помещаем в столбцы с пустым именем
        SelectColumn col;
        col.name = ""; // маркер агрегата
        col.alias = "";
        $$ = std::move(col);
        // TODO: добавить aggregates в SelectQuery
    }
;

aggregate_expr:
    SUM LPAREN column_name_or_star RPAREN
    {
        AggregateExpr agg;
        agg.func = "SUM";
        agg.column = $3;
        $$ = agg;
    }
  | COUNT LPAREN column_name_or_star RPAREN
    {
        AggregateExpr agg;
        agg.func = "COUNT";
        agg.column = $3;
        $$ = agg;
    }
  | AVG LPAREN column_name_or_star RPAREN
    {
        AggregateExpr agg;
        agg.func = "AVG";
        agg.column = $3;
        $$ = agg;
    }
;

column_name_or_star:
    IDENTIFIER { $$ = std::move($1); }
    | STAR     { $$ = "*"; }
;

where_opt:
    /* empty */    { $$ = nullptr; }
    | WHERE condition { $$ = std::move($2); }
;

/* Условия */
condition:
    or_condition { $$ = std::move($1); }
;

or_condition:
    and_condition
    {
        $$ = std::move($1);
    }
  | or_condition OR and_condition
    {
        $$ = std::make_unique<OrOp>(std::move($1), std::move($3));
    }
;

and_condition:
    comparison
    {
        $$ = std::move($1);
    }
  | and_condition AND comparison
    {
        $$ = std::make_unique<AndOp>(std::move($1), std::move($3));
    }
;

comparison:
    expr EQ expr
        { $$ = std::make_unique<BinaryOp>("==", std::move($1), std::move($3)); }
    | expr NE expr
        { $$ = std::make_unique<BinaryOp>("!=", std::move($1), std::move($3)); }
    | expr LT expr
        { $$ = std::make_unique<BinaryOp>("<",  std::move($1), std::move($3)); }
    | expr GT expr
        { $$ = std::make_unique<BinaryOp>(">",  std::move($1), std::move($3)); }
    | expr LE expr
        { $$ = std::make_unique<BinaryOp>("<=", std::move($1), std::move($3)); }
    | expr GE expr
        { $$ = std::make_unique<BinaryOp>(">=", std::move($1), std::move($3)); }
    | expr BETWEEN expr AND expr
        { $$ = std::make_unique<BetweenOp>(std::move($1), std::move($3), std::move($5)); }
    | expr LIKE expr
        { $$ = std::make_unique<LikeOp>(std::move($1), std::move($3)); }
    | LPAREN condition RPAREN
        { $$ = std::move($2); }
;

expr:
    literal      { $$ = std::move($1); }
    | column_ref { $$ = std::move($1); }
;

literal:
    INTEGER  { $$ = std::make_unique<Literal>(Value($1)); }
    | STRING { $$ = std::make_unique<Literal>(Value($1)); }
    | NULL_  { $$ = std::make_unique<Literal>(Value()); } // Value по умолчанию – NULL
;

column_ref:
    IDENTIFIER { $$ = std::make_unique<ColumnRef>($1); }
;

/* INSERT */
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

column_list:
    IDENTIFIER
    {
        $$ = std::vector<std::string>();
        $$.push_back($1);
    }
    | column_list COMMA IDENTIFIER { $$ = $1; $$.push_back($3); }
;

values_list:
    LPAREN value_list RPAREN
    {
        $$ = std::vector<std::vector<std::unique_ptr<ASTNode>>>();
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
        $$ = std::vector<std::unique_ptr<ASTNode>>();
        $$.push_back(std::move($1));
    }
    | value_list COMMA expr
    {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

/* UPDATE */
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
        $$ = std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>();
        $$.emplace_back($1, std::move($3));
    }
    | assignment_list COMMA IDENTIFIER ASSIGN expr
    {
        $$ = std::move($1);
        $$.emplace_back($3, std::move($5));
    }
;

/* DELETE */
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
        q->columns = std::move($5);
        $$ = std::move(q);
    }
;

column_definitions:
    column_definition
    {
        $$ = std::vector<ColumnSpec>();
        $$.push_back(std::move($1));
    }
    | column_definitions COMMA column_definition
    {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

column_definition:
    IDENTIFIER type_spec not_null_opt indexed_opt default_opt
    {
        ColumnSpec col;
        col.name = $1;
        col.type = $2;
        col.notNull = $3;
        col.indexed = $4;
        col.defaultValue = $5;
        $$ = std::move(col);
    }
;

type_spec:
    INT_TYPE    { $$ = ColType::INT; }
    | STRING_TYPE { $$ = ColType::STRING; }
;

not_null_opt:
    /* empty */     { $$ = false; }
    | NOT NULL_     { $$ = true; }
;

indexed_opt:
    /* empty */     { $$ = false; }
    | INDEXED       { $$ = true; }
;

default_opt:
    /* empty */             { $$ = Value(); }       // NULL
    | DEFAULT literal
    {
        auto lit = dynamic_cast<Literal*>($2.get());
        $$ = lit ? lit->value : Value();
    }
;

/* DROP TABLE */
drop_table_stmt:
    DROP TABLE IDENTIFIER
    {
        auto q = std::make_unique<DropTableQuery>($3);
        $$ = std::move(q);
    }
;

/* CREATE DATABASE */
create_database_stmt:
    CREATE DATABASE IDENTIFIER
    {
        auto q = std::make_unique<CreateDatabaseQuery>($3);
        $$ = std::move(q);
    }
;

/* DROP DATABASE */
drop_database_stmt:
    DROP DATABASE IDENTIFIER
    {
        auto q = std::make_unique<DropDatabaseQuery>($3);
        $$ = std::move(q);
    }
;

/* USE */
use_stmt:
    USE IDENTIFIER
    {
        auto q = std::make_unique<UseQuery>($2);
        $$ = std::move(q);
    }
;

%%

void yy::parser::error(const location& loc, const std::string& msg) {
    errorMsg = "Parse error at line " + std::to_string(loc.begin.line) +
               ", column " + std::to_string(loc.begin.column) + ": " + msg;
}
