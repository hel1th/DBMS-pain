#ifndef DBMS_PAIN_ERROR_H
#define DBMS_PAIN_ERROR_H
#include <stdexcept>

struct DbException : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ParseError : DbException {
    using DbException::DbException;
};

struct SemanticError : DbException {
    using DbException::DbException;
};

struct StorageError : DbException {
    using DbException::DbException;
};

struct IndexError : DbException {
    using DbException::DbException;
};

#endif // DBMS_PAIN_ERROR_H
