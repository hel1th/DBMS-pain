#ifndef DBMS_PAIN_SERIALIZER_H
#define DBMS_PAIN_SERIALIZER_H

#include "engine/Schema.h"
#include "utils/Value.h"
#include <cstdint>
#include <vector>

class Serializer {
public:
  static std::vector<char> serialize(const std::vector<Value> &record,
                                     const Schema &schema);

  static std::vector<Value> deserialize(const char *data, size_t size,
                                        const Schema &schema);

  static size_t serializedSize(const std::vector<Value> &record,
                               const Schema &schema);

  static size_t maxSerializedSize(const Schema &schema);

  static bool isValid(const char *data, size_t size, const Schema &schema);

private:
  static size_t getNullBitmapSize(const Schema &schema);
  
  static void setNullBit(char *bitmap, size_t columnIndex);
  
  static bool isNull(const char *bitmap, size_t columnIndex);

  static void writeInt32(std::vector<char> &bytes, int32_t value);
  static void writeDouble(std::vector<char> &bytes, double value);
  static void writeString(std::vector<char> &bytes, const std::string &value);
  
  static int32_t readInt32(const char *data);
  static uint32_t readUint32(const char *data);
  static double readDouble(const char *data);
  static std::string readString(const char *data, uint32_t &bytesRead);
};

#endif // DBMS_PAIN_SERIALIZER_H