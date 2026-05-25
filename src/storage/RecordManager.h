#ifndef DBMS_PAIN_RECORDMANAGER_H
#define DBMS_PAIN_RECORDMANAGER_H

#include <functional>
#include <optional>

#include "PageManager.h"
#include "Serializer.h"
#include "engine/Schema.h"
#include "utils/Value.h"

struct RecordID {
    PageID_t pageID;
    int16_t  slotID;
    
    bool operator==(const RecordID& other) const {
        return pageID == other.pageID && slotID == other.slotID;
    }
    
    bool operator!=(const RecordID& other) const {
        return !(*this == other);
    }
};

class RecordManager {
public:
    RecordManager(PageManager& pm, const Schema& schema);
    
    RecordID insert(const std::vector<Value>& record);
    std::vector<Value> fetch(RecordID recordID);
    RecordID update(RecordID recordID, const std::vector<Value>& record);
    void remove(RecordID recordID);

    void scan(std::function<void(RecordID, const std::vector<Value>&)> callback);
    
    bool isValid(RecordID recordID) const;
    size_t getTotalRecordCount() const;

private:
    PageManager& pm_;
    const Schema& schema_;
    
    int16_t getSlotCount(const Page& page) const;
    int16_t getFreeOffset(const Page& page) const;
    void setFreeOffset(Page& page, int16_t freeOffset);
    void setRecordCount(Page& page, int16_t recordCount);
    
    struct Slot {
        int16_t offset;
        int16_t size;
        int32_t reserved;
    };
    
    Slot getSlot(const Page& page, int16_t slotIndex) const;
    void setSlot(Page& page, int16_t slotIndex, const Slot& slot);
    void removeSlot(Page& page, int16_t slotIndex);  // size = 0
    
    PageID_t findPageWithSpace(size_t neededBytes);
    int16_t findFreeSlotIndex(const Page& page) const;
    int16_t findInsertPosition(const Page& page, size_t recordSize) const;
    
    void appendRecord(Page& page, const std::vector<char>& data, RecordID& recordID);
    void compactPage(Page& page);
    bool shouldCompact(const Page& page) const;  // Порог фрагментации
    
    bool isPageEmpty(const Page& page) const;
    void clearPage(Page& page);
};

#endif // DBMS_PAIN_RECORDMANAGER_H