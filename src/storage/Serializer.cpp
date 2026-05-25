#include "Serializer.h"
#include <cstring>
#include <stdexcept>


std::vector<char> Serializer::serialize(const std::vector<Value> &record,
                                        const Schema &schema) {
  const size_t columnCount = schema.columns.size();
  
  if (record.size() != columnCount) {
    throw std::runtime_error(
      "Serializer::serialize: Column count mismatch. Expected " +
      std::to_string(columnCount) + ", got " + std::to_string(record.size()));
  }
  
  for (size_t i = 0; i < columnCount; i++) {
    if (schema.columns[i].notNull && val::isNull(record[i])) {
      throw std::runtime_error(
        "Serializer::serialize: NULL value in NOT NULL column " + schema.columns[i].name);
    }
    
    // Проверка соответствия типов (не NULL значения)
    if (!val::isNull(record[i])) {
      if (schema.columns[i].type == ColType::INT && !val::isInt(record[i])) {
        throw std::runtime_error(
          "Serializer::serialize: Type mismatch at column " + std::to_string(i) +
          " (" + schema.columns[i].name + "). Expected INT, got non-INT");
      }
      if (schema.columns[i].type == ColType::STRING && !val::isString(record[i])) {
        throw std::runtime_error(
          "Serializer::serialize: Type mismatch at column " + std::to_string(i) +
          " (" + schema.columns[i].name + "). Expected STRING, got non-STRING");
      }
    }
  }
  
  std::vector<char> bytes;
  
  size_t bitmapSize = getNullBitmapSize(schema);
  bytes.resize(bitmapSize, 0);
  
  for (size_t i = 0; i < columnCount; i++) {
    if (val::isNull(record[i])) {
      setNullBit(bytes.data(), i);
    } else {
      if (schema.columns[i].type == ColType::INT) {
        int intValue = val::getInt(record[i]);
        writeInt32(bytes, intValue);  // 4 байта
      } else if (schema.columns[i].type == ColType::STRING) {
        std::string strValue = val::getString(record[i]);
        writeString(bytes, strValue);
      }
    }
  }
  
  return bytes;
}

std::vector<Value> Serializer::deserialize(const char *data, size_t size,
                                          const Schema &schema) {
  const size_t columnCount = schema.columns.size();
  size_t bitmapSize = getNullBitmapSize(schema);
  
  if (data == nullptr || size < bitmapSize) {
    throw std::runtime_error(
      "Serializer::deserialize: Data too small for null bitmap");
  }
  
  const char *ptr = data;
  const char *end = data + size;
  
  const char *bitmap = ptr;
  ptr += bitmapSize;
  
  std::vector<Value> result;
  result.reserve(columnCount);
  
  for (size_t i = 0; i < columnCount; i++) {
    if (isNull(bitmap, i)) {
      result.push_back(std::nullopt);
    } else {
      if (schema.columns[i].type == ColType::INT) {
        if (ptr + 4 > end) {  // 4 байта для int32_t
          throw std::runtime_error(
            "Serializer::deserialize: Not enough data for INT at column " +
            std::to_string(i));
        }
        int32_t val = readInt32(ptr);
        ptr += 4;
        result.push_back(static_cast<int>(val));
      } else if (schema.columns[i].type == ColType::STRING) {
        if (ptr + 4 > end) {
          throw std::runtime_error(
            "Serializer::deserialize: Not enough data for string length at column " +
            std::to_string(i));
        }
        uint32_t len = readUint32(ptr);
        ptr += 4;
        if (ptr + len > end) {
          throw std::runtime_error(
            "Serializer::deserialize: Not enough data for string content at column " +
            std::to_string(i));
        }
        std::string str(ptr, len);
        ptr += len;
        result.push_back(str);
      } else {
        throw std::runtime_error(
          "Serializer::deserialize: Unknown column type in schema");
      }
    }
  }
  
  return result;
}

size_t Serializer::serializedSize(const std::vector<Value> &record,
                                  const Schema &schema) {
  const size_t columnCount = schema.columns.size();
  
  if (record.size() != columnCount) {
    throw std::runtime_error(
      "Serializer::serializedSize: Column count mismatch");
  }
  
  size_t total = getNullBitmapSize(schema);
  
  for (size_t i = 0; i < columnCount; i++) {
    if (val::isNull(record[i])) {
      continue;  // нулл значения не занимают места (кроме бита в bitmap)
    }
    
    if (schema.columns[i].type == ColType::INT) {
      total += 4;  // int32_t = 4 байта
    } else if (schema.columns[i].type == ColType::STRING) {
      std::string strValue = val::getString(record[i]);
      total += 4 + strValue.size();  // длина 4 + данные
    }
  }
  
  return total;
}

size_t Serializer::maxSerializedSize(const Schema &schema) {
  const size_t columnCount = schema.columns.size();
  size_t total = getNullBitmapSize(schema);
  
  for (size_t i = 0; i < columnCount; i++) {
    if (schema.columns[i].type == ColType::INT) {
      total += 4;
    } else if (schema.columns[i].type == ColType::STRING) {
      total += 4 + 65535;  // Максимальная длина строки (64KB)
    }
  }
  
  return total;
}

bool Serializer::isValid(const char *data, size_t size, const Schema &schema) {
  if (data == nullptr) {
    return false;
  }
  
  size_t bitmapSize = getNullBitmapSize(schema);
  if (size < bitmapSize) {
    return false;
  }
  
  const char *ptr = data;
  const char *end = data + size;
  const char *bitmap = ptr;
  ptr += bitmapSize;
  
  try {
    for (size_t i = 0; i < schema.columns.size(); i++) {
      if (isNull(bitmap, i)) {
        continue;
      }
      
      if (schema.columns[i].type == ColType::INT) {
        if (ptr + 4 > end) return false;  // 4 байта
        ptr += 4;
      } else if (schema.columns[i].type == ColType::STRING) {
        if (ptr + 4 > end) return false;
        uint32_t len = readUint32(ptr);
        ptr += 4;
        if (ptr + len > end) return false;
        ptr += len;
      } else {
        return false;
      }
    }
  } catch (...) {
    return false;
  }
  
  return true;
}


size_t Serializer::getNullBitmapSize(const Schema &schema) {
  const size_t columnCount = schema.columns.size();
  return (columnCount + 7) / 8;
}

void Serializer::setNullBit(char *bitmap, size_t columnIndex) {
  size_t byteIndex = columnIndex / 8;
  size_t bitIndex = columnIndex % 8;
  bitmap[byteIndex] |= static_cast<char>(1 << bitIndex);
}

bool Serializer::isNull(const char *bitmap, size_t columnIndex) {
  size_t byteIndex = columnIndex / 8;
  size_t bitIndex = columnIndex % 8;
  return (static_cast<unsigned char>(bitmap[byteIndex]) & (1 << bitIndex)) != 0;
}

void Serializer::writeInt32(std::vector<char> &bytes, int32_t value) {
  uint32_t uvalue = static_cast<uint32_t>(value);
  bytes.push_back(static_cast<char>(uvalue >> 0));
  bytes.push_back(static_cast<char>(uvalue >> 8));
  bytes.push_back(static_cast<char>(uvalue >> 16));
  bytes.push_back(static_cast<char>(uvalue >> 24));
}

int32_t Serializer::readInt32(const char *data) {
  return static_cast<int32_t>(
      static_cast<uint32_t>(static_cast<uint8_t>(data[0])) |
      static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 8 |
      static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 16 |
      static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 24);
}

uint32_t Serializer::readUint32(const char *data) {
  return static_cast<uint32_t>(
      static_cast<uint32_t>(static_cast<uint8_t>(data[0])) |
      static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 8 |
      static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 16 |
      static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 24);
}

void Serializer::writeString(std::vector<char> &bytes, const std::string &value) {
  uint32_t len = static_cast<uint32_t>(value.size());
  bytes.push_back(static_cast<char>(len >> 0));
  bytes.push_back(static_cast<char>(len >> 8));
  bytes.push_back(static_cast<char>(len >> 16));
  bytes.push_back(static_cast<char>(len >> 24));

  bytes.insert(bytes.end(), value.begin(), value.end());
}

std::string Serializer::readString(const char *data, uint32_t &bytesRead) {
  uint32_t len = readUint32(data);
  if (len > 1024 * 1024 * 10) {  // 10 MB лимит
    throw std::runtime_error("String too long: " + std::to_string(len));
  }
  bytesRead = 4 + len;
  return std::string(data + 4, len);
}