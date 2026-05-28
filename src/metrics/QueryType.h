#ifndef DBMS_PAIN_QUERY_TYPE_H
#define DBMS_PAIN_QUERY_TYPE_H

#include <string>

enum class QueryType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    OTHER
};

inline std::string queryTypeToString(QueryType type) {
    switch (type) {
        case QueryType::SELECT: return "SELECT";
        case QueryType::INSERT: return "INSERT";
        case QueryType::UPDATE: return "UPDATE";
        case QueryType::DELETE: return "DELETE";
        default: return "OTHER";
    }
}

inline QueryType queryTypeFromString(const std::string& str) {
    if (str == "SELECT") return QueryType::SELECT;
    if (str == "INSERT") return QueryType::INSERT;
    if (str == "UPDATE") return QueryType::UPDATE;
    if (str == "DELETE") return QueryType::DELETE;
    return QueryType::OTHER;
}

#endif