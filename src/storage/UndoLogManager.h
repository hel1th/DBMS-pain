#ifndef DBMS_PAIN_UNDOLOGMANAGER_h
#define DBMS_PAIN_UNDOLOGMANAGER_H
#include <string>
#include <cstdint>
#include <vector>
#include <fstream>
#include <mutex>

enum class RevertActionType : uint8_t {
    REVERT_INSERT = 1,
    REVERT_DELETE = 2,
    REVERT_UPDATE = 3
};

struct UndoLogRecord {
    uint64_t timeMS;
    std::string tableName;
    RevertActionType actionType;
    std::vector<uint8_t> key;
    std::vector<uint8_t> oldRowData;
};

class UndoLogManager {
    public:

        explicit UndoLogManager(const std::string& LogfilePath);
        ~UndoLogManager();

        UndoLogManager(const UndoLogManager&) = delete;
        UndoLogManager& operator=(const UndoLogManager&) = delete;

        void logUndoInsert(const std::string& tableName, uint64_t timeMs, std::vector<uint8_t>);
        void logUndoDelete(const std::string& tableName, uint64_t timeMs, uint64_t rowId, std::vector<uint8_t>& keys, std::vector<uint8_t>& oldRowData);
        void logUndoUpdate(const std::string& tableName, uint64_t timeMs, std::vector<uint8_t> keys, std::vector<uint8_t> oldRowData);

        std::vector<UndoLogRecord> getRecordsToRevert(const std::string& tableName, uint64_t timeMs);

        void truncateLog(uint64_t timeMs);

        static uint64_t getCurrentTimeMs();

    private:
        std::string filePath_;
        std::fstream logFile_;
        std::mutex mutex_;

        void writeRecord(const UndoLogRecord& record);
        UndoLogRecord readRecord(std::ifstream& in);

        void writeBinaryString(std::ostream& out, const std::string& str);
        void writebBinary(std::ostream& out, const std::vector<uint8_t>& bytes);

        std::string readBinaryString(std::istream& in);
        std::vector<uint8_t> readBinary(std::istream& in);
};

#endif // DBMS_PAIN_UNDOLOGMANAGER_H
