#ifndef DBMS_PAIN_VALUE_H
#define DBMS_PAIN_VALUE_H

#include <optional>
#include <string>
#include <variant>


// Value.h — этот тип используется везде
using Value = std::optional<std::variant<int, std::string>>;
// nullptr/nullopt = NULL
// variant<int>    = целое
// variant<string> = строка

#endif // DBMS_PAIN_VALUE_H