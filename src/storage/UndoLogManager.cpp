#include "UndoLogManager.h"
#include <filesystem>
#include "utils/Error.h"


UndoLogManager::UndoLogManager(const std::string& LogfilePath) : filePath_(LogfilePath) {
    std::lock_guard<std::mutex> lock(this->mutex_);

    std::filesystem::path path(LogfilePath);

    if (!std::filesystem::exists(path)) {
        std::ofstream file(LogfilePath, std::ios::binary);
        if (!file.is_open()) {
            throw UndoLogError("Failed to create log file:" + this->filePath_);
        }
    }
    this->logFile_.open(LogfilePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!this->logFile_.is_open()) {
        throw UndoLogError("Failed to open undo log file" + this->filePath_);
    }
}

UndoLogManager::~UndoLogManager() {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if (this->logFile_.is_open()) {
        this->logFile_.flush(); // все чо в буфере осталось - пишем
        this->logFile_.close();
    }
}

uint64_t UndoLogManager::getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return static_cast<uint64_t>(millisecs.count());
}

std::vector<UndoLogRecord> UndoLogManager::getRecordsToRevert(const std::string& tableName,
                                                              uint64_t timeMs) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    std::vector<UndoLogRecord> recordsToRevert;

    std::ifstream file(this->filePath_, std::ios::binary);
    if (!file.is_open()) {
        return recordsToRevert;
    }
    file.seekg(0, std::ios::end);
    std::streampos currentPos = file.tellg();
    while (currentPos > 0) {
        file.seekg(currentPos - static_cast<std::streamoff>(sizeof(uint32_t)));
        uint32_t recordSize = 0;
        file.read(reinterpret_cast<char*>(&recordSize), sizeof(recordSize));
        std::streampos recordStartPos =
                currentPos - static_cast<std::streamoff>(recordSize + sizeof(uint32_t));
        file.seekg(recordStartPos);
        UndoLogRecord record = readRecord(file);
        if (record.timeMS < timeMs) {
            break;
        }
        if (record.tableName == tableName) {
            recordsToRevert.push_back(record);
        }
        currentPos = recordStartPos;
    }
    return recordsToRevert;
}

void UndoLogManager::truncateLog(uint64_t timeMs) {
    std::lock_guard<std::mutex> lock(this->mutex_);

    std::ifstream file(this->filePath_, std::ios::binary);
    if (!file.is_open()) {
        throw UndoLogError("Cannot open log file!");
    }
    file.seekg(0, std::ios::end);
    std::streampos currentPos = file.tellg();
    std::streampos truncatePos = currentPos;

    while (currentPos > 0) {
        file.seekg(currentPos - static_cast<std::streamoff>(sizeof(uint32_t)));
        uint32_t recordSize = 0;
        file.read(reinterpret_cast<char*>(&recordSize), sizeof(recordSize));
        std::streampos recordStartPos =
                currentPos - static_cast<std::streamoff>(recordSize + sizeof(uint32_t));

        file.seekg(recordStartPos);
        uint64_t recordTime = 0;
        file.read(reinterpret_cast<char*>(&recordTime), sizeof(recordTime));
        if (recordTime < timeMs) {
            truncatePos = currentPos;
            break;
        }
        truncatePos = recordStartPos;
        currentPos = recordStartPos;
    }
    file.close();

    if (this->logFile_.is_open()) {
        this->logFile_.close();
    }

    std::filesystem::resize_file(this->filePath_, truncatePos);

    this->logFile_.open(this->filePath_, std::ios::in | std::ios::out | std::ios::binary);
}

void UndoLogManager::writeBinaryString(std::ostream& out, const std::string& str) {
    out.clear();

    uint32_t strLen = str.size();
    out.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));

    if (strLen > 0) {
        out.write(str.data(), strLen);
    }
    out.flush();
}

void UndoLogManager::writeBinary(std::ostream& out, const std::vector<uint8_t>& bytes) {
    out.clear();

    uint32_t vecLen = bytes.size();
    out.write(reinterpret_cast<const char*>(&vecLen), sizeof(vecLen));

    if (vecLen > 0) {
        out.write(reinterpret_cast<const char*>(bytes.data()), vecLen);
    }
    out.flush();
}

void UndoLogManager::writeRecord(const UndoLogRecord& record) {
    this->logFile_.clear();

    this->logFile_.seekp(0, std::ios::end);
    std::streampos startPos = this->logFile_.tellp();

    this->logFile_.write(reinterpret_cast<const char*>(&record.timeMS), sizeof(record.timeMS));
    writeBinaryString(this->logFile_, record.tableName);
    this->logFile_.write(reinterpret_cast<const char*>(&record.actionType),
                         sizeof(record.actionType));
    writeBinary(this->logFile_, record.keys);
    writeBinary(this->logFile_, record.oldRowData);

    std::streampos endPos = this->logFile_.tellp();
    uint32_t recordSize = endPos - startPos;
    logFile_.write(reinterpret_cast<char*>(&recordSize), sizeof(recordSize));
    this->logFile_.flush();
}

std::string UndoLogManager::readBinaryString(std::istream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));

    if (len == 0) {
        return "";
    }

    std::string str(len, '\0');
    in.read(&str[0], len);
    return str;
}

std::vector<uint8_t> UndoLogManager::readBinary(std::istream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));

    if (len == 0) {
        return {};
    }

    std::vector<uint8_t> bytes(len);
    in.read(reinterpret_cast<char*>(bytes.data()), len);
    return bytes;
}

UndoLogRecord UndoLogManager::readRecord(std::ifstream& in) {
    UndoLogRecord record;

    in.read(reinterpret_cast<char*>(&record.timeMS), sizeof(record.timeMS));
    record.tableName = readBinaryString(in);

    uint8_t action;
    in.read(reinterpret_cast<char*>(&action), sizeof(action));
    record.actionType = static_cast<RevertActionType>(action);

    record.keys = readBinary(in);
    record.oldRowData = readBinary(in);

    uint32_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    return record;
}

void UndoLogManager::logUndoInsert(const std::string& tableName, uint64_t timeMs,
                                   std::vector<uint8_t>& keys) {
    std::lock_guard<std::mutex> lock(this->mutex_);

    UndoLogRecord record;
    record.timeMS = timeMs;
    record.tableName = tableName;
    record.actionType = RevertActionType::REVERT_INSERT;
    record.keys = keys;
    record.oldRowData = {};

    writeRecord(record);
}

void UndoLogManager::logUndoDelete(const std::string& tableName, uint64_t timeMs,
                                   std::vector<uint8_t>& keys, std::vector<uint8_t>& oldRowData) {
    std::lock_guard<std::mutex> lock(this->mutex_);

    UndoLogRecord record;
    record.timeMS = timeMs;
    record.tableName = tableName;
    record.actionType = RevertActionType::REVERT_DELETE;
    record.keys = keys;
    record.oldRowData = oldRowData;

    writeRecord(record);
}

void UndoLogManager::logUndoUpdate(const std::string& tableName, uint64_t timeMs,
                                   std::vector<uint8_t>& keys, std::vector<uint8_t>& oldRowData) {
    std::lock_guard<std::mutex> lock(this->mutex_);

    UndoLogRecord record;
    record.timeMS = timeMs;
    record.tableName = tableName;
    record.actionType = RevertActionType::REVERT_UPDATE;
    record.keys = keys;
    record.oldRowData = oldRowData;

    writeRecord(record);
}
