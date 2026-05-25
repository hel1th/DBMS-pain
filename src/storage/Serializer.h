#ifndef DBMS_PAIN_SERIALIZER_H
#define DBMS_PAIN_SERIALIZER_H

#include "engine/Schema.h"
#include "utils/Value.h"
#include <cstdint>
#include <vector>

class Serializer {
public:
  // Упаковать запись в байты
  static std::vector<char> serialize(const std::vector<Value> &record,
                                     const Schema &schema);

  // Распаковать байты в запись
  static std::vector<Value> deserialize(const char *data, size_t size,
                                        const Schema &schema);

  // Сколько байт займёт запись (для проверки что влезет на страницу)
  static size_t serialized_size(const std::vector<Value> &record,
                                const Schema &schema);
};

#endif // DBMS_PAIN_SERIALIZER_H