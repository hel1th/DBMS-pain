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
    
    RecordID Insert(const std::vector<Value>& record);
    std::vector<Value> Fetch(RecordID recordID);
    void Update(RecordID recordID, const std::vector<Value>& record);
    void Remove(RecordID recordID);

    void Scan(std::function<void(RecordID, const std::vector<Value>&)> callback);
    
    bool IsValid(RecordID recordID) const;
    size_t GetRecordCount() const;

private:
    PageManager& pm_;
    const Schema& schema_;
    
    int16_t GetSlotCount(const Page& page) const;
    int16_t GetFreeOffset(const Page& page) const;
    void SetFreeOffset(Page& page, int16_t freeOffset);
    void SetRecordCount(Page& page, int16_t recordCount);
    
    struct Slot {
        int16_t offset;
        int16_t size;
        int32_t reserved;
    };
    
    Slot GetSlot(const Page& page, int16_t slotIndex) const;
    void SetSlot(Page& page, int16_t slotIndex, const Slot& slot);
    void RemoveSlot(Page& page, int16_t slotIndex);  // size = 0
    
    PageID_t FindPageWithSpace(size_t neededBytes);
    int16_t FindFreeSlotIndex(const Page& page) const;
    int16_t FindInsertPosition(const Page& page, size_t recordSize) const;
    
    void AppendRecord(Page& page, const std::vector<char>& data, RecordID& recordID);
    void CompactPage(Page& page);
    bool ShouldCompact(const Page& page) const;  // Порог фрагментации
    
    bool IsPageEmpty(const Page& page) const;
    void ClearPage(Page& page);
};

#endif // DBMS_PAIN_RECORDMANAGER_H