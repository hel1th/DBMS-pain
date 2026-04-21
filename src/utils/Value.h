#ifndef DBMS_PAIN_VALUE_H
#define DBMS_PAIN_VALUE_H

#include <optional>
#include <variant>
#include <string>

// NULL = nullopt
// INT  = variant содержит int
// STR  = variant содержит string
using Value = std::optional<std::variant<int, std::string>>;

#endif // DBMS_PAIN_VALUE_H