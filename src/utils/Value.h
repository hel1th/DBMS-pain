#ifndef DBMS_PAIN_VALUE_H
#define DBMS_PAIN_VALUE_H

#include <optional>
#include <string>
#include <variant>

// NULL = nullopt
// INT  = variant содержит int
// STR  = variant содержит string
using Value = std::optional<std::variant<int, std::string>>;


namespace val {
    inline bool isNull(const Value& v) { return !v.has_value(); }
    inline bool isInt(const Value& v) { return v && std::holds_alternative<int>(*v); }
    inline bool isString(const Value& v) { return v && std::holds_alternative<std::string>(*v); }
    inline int getInt(const Value& v) { return std::get<int>(*v); }
    inline std::string getString(const Value& v) { return std::get<std::string>(*v); }
} // namespace val

// cmp of two Value (for WHERE)
// NULL != NULL, NULL is not comparable, always returns false
// STRING and INT is not comparable, always returns false
inline bool valueLess(const Value& a, const Value& b) {
    if (!a || !b)
        return false;

    if (val::isInt(a) && val::isInt(b))
        return val::getInt(a) < val::getInt(b);

    if (val::isString(a) && val::isString(b))
        return val::getString(a) < val::getString(b);


    return false;
}

inline bool valueEqual(const Value& a, const Value& b) {
    if (!a && !b)
        return true;

    if (!a || !b)
        return false;

    return *a == *b;
}


#endif // DBMS_PAIN_VALUE_H
