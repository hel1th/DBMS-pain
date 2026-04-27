#ifndef DBMS_PAIN_SERIALIZER_H
#define DBMS_PAIN_SERIALIZER_H

#include "engine/Schema.h"
#include "utils/Value.h"
#include <cstdint>
#include <vector>

class Serializer {
public:
  static std::vector<char> Serialize(const std::vector<Value> &record,
                                     const Schema &schema);

  static std::vector<Value> Deserialize(const char *data, size_t size,
                                        const Schema &schema);

  static size_t SerializedSize(const std::vector<Value> &record,
                               const Schema &schema);

  static size_t MaxSerializedSize(const Schema &schema);

  static bool IsValid(const char *data, size_t size, const Schema &schema);

private:
  static size_t GetNullBitmapSize(const Schema &schema);
  
  static void SetNullBit(char *bitmap, size_t columnIndex);
  
  static bool IsNull(const char *bitmap, size_t columnIndex);

  static bool IsIntColumn(const ColumnDef& col) { return col.type == ColType::INT; }
  static bool IsStringColumn(const ColumnDef& col) { return col.type == ColType::STRING; }

  static void WriteInt32(std::vector<char> &bytes, int32_t value);
  static void WriteDouble(std::vector<char> &bytes, double value);
  static void WriteString(std::vector<char> &bytes, const std::string &value);
  
  static int32_t ReadInt32(const char *data);
  static uint32_t ReadUint32(const char *data);
  static double ReadDouble(const char *data);
  static std::string ReadString(const char *data, uint32_t &bytesRead);
};

#endif // DBMS_PAIN_SERIALIZER_H