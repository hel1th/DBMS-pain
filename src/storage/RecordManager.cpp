#include "RecordManager.h"
#include <cstring>
#include <stdexcept>


// Константы страницы
constexpr int16_t PAGE_HEADER_SIZE = 16;
constexpr int16_t PAGE_NEXT_PAGE_OFFSET = 0;      // 4 байта (PageID_t)
constexpr int16_t PAGE_RECORD_COUNT_OFFSET = 4;   // 2 байта (int16_t)
constexpr int16_t PAGE_FREE_OFFSET_OFFSET = 6;    // 2 байта (int16_t)
constexpr int16_t PAGE_FLAGS_OFFSET = 8;          // 4 байта (int32_t)
constexpr int16_t SLOT_SIZE = 8;  // offset(2) + size(2) + reserved(4)
constexpr int16_t INVALID_SLOT = -1;


RecordManager::RecordManager(PageManager& pm, const Schema& schema)
    : pm_(pm), schema_(schema) {}


RecordID RecordManager::insert(const std::vector<Value>& record) {
    std::vector<char> data = Serializer::serialize(record, schema_);
    size_t recordSize = data.size();
    
    PageID_t pageID = findPageWithSpace(recordSize);
    Page page = pm_.readPage(pageID);

    int16_t insertPosition;
    try {
        insertPosition = findInsertPosition(page, recordSize);
    } catch (const std::runtime_error&) {
        compactPage(page);
        insertPosition = findInsertPosition(page, recordSize);
    }
    
    int16_t slotIndex = findFreeSlotIndex(page);
    RecordID recordID;
    recordID.pageID = pageID;
    
    if (slotIndex == INVALID_SLOT) {
        slotIndex = getSlotCount(page);
        setRecordCount(page, getSlotCount(page) + 1);
    }
    
    std::memcpy(page.data() + insertPosition, data.data(), recordSize);
    
    Slot slot{insertPosition, static_cast<int16_t>(recordSize), 0};
    setSlot(page, slotIndex, slot);
    setFreeOffset(page, insertPosition);
    
    pm_.writePage(pageID, page);
    
    recordID.slotID = slotIndex;
    return recordID;
}

std::vector<Value> RecordManager::fetch(RecordID recordID) {
    if (!isValid(recordID)) {
        throw std::runtime_error("RecordManager::fetch: Invalid RecordID");
    }
    
    Page page = pm_.readPage(recordID.pageID);
    Slot slot = getSlot(page, recordID.slotID);
    
    if (slot.size == 0) {
        throw std::runtime_error("RecordManager::fetch: Record has been deleted");
    }
    
    const char* dataPtr = page.data() + slot.offset;
    return Serializer::deserialize(dataPtr, static_cast<size_t>(slot.size), schema_);
}

RecordID RecordManager::update(RecordID recordID, const std::vector<Value>& record) {
    if (!isValid(recordID)) {
        throw std::runtime_error("RecordManager::update: Invalid RecordID");
    }
    
    std::vector<char> newData = Serializer::serialize(record, schema_);
    size_t newSize = newData.size();
    
    Page page = pm_.readPage(recordID.pageID);
    Slot oldSlot = getSlot(page, recordID.slotID);
    
    if (oldSlot.size == 0) {
        throw std::runtime_error("RecordManager::update: Cannot update deleted record");
    }
    
    if (static_cast<size_t>(oldSlot.size) == newSize) {
        std::memcpy(page.data() + oldSlot.offset, newData.data(), newSize);
        pm_.writePage(recordID.pageID, page);
        return recordID;
    }
    
    remove(recordID);
    return insert(record);
}

void RecordManager::remove(RecordID recordID) {
    if (!isValid(recordID)) {
        throw std::runtime_error("RecordManager::remove: Invalid RecordID");
    }
    
    Page page = pm_.readPage(recordID.pageID);
    removeSlot(page, recordID.slotID);
    pm_.writePage(recordID.pageID, page);
    
    if (isPageEmpty(page)) {
        pm_.freePage(recordID.pageID);
    }
}

void RecordManager::scan(std::function<void(RecordID, const std::vector<Value>&)> callback) {
    int32_t pageCount = pm_.pageCount();
    
    for (PageID_t pageID = 1; pageID < pageCount; ++pageID) {
        try {
            Page page = pm_.readPage(pageID);
            int16_t recordCount = getSlotCount(page);
            
            for (int16_t slotID = 0; slotID < recordCount; ++slotID) {
                Slot slot = getSlot(page, slotID);
                if (slot.size > 0) {
                    RecordID recordID{pageID, slotID};
                    try {
                        std::vector<Value> values = fetch(recordID);
                        callback(recordID, values);
                    } catch (const std::exception&) {
                        // Пропускаем повреждённые записи
                    }
                }
            }
        } catch (const std::exception&) {
            // Пропускаем повреждённые страницы
        }
    }
}

bool RecordManager::isValid(RecordID recordID) const {
    if (recordID.pageID <= 0 || recordID.pageID >= pm_.pageCount()) {
        return false;
    }
    
    if (recordID.slotID < 0) {
        return false;
    }
    
    try {
        Page page = pm_.readPage(recordID.pageID);
        int16_t recordCount = getSlotCount(page);
        
        if (recordID.slotID >= recordCount) {
            return false;
        }
        
        Slot slot = getSlot(page, recordID.slotID);
        return slot.size > 0;
    } catch (...) {
        return false;
    }
}

size_t RecordManager::getTotalRecordCount() const {
    size_t total = 0;
    int32_t pageCount = pm_.pageCount();
    
    for (PageID_t pageID = 1; pageID < pageCount; ++pageID) {
        try {
            Page page = pm_.readPage(pageID);
            total += static_cast<size_t>(getSlotCount(page));
        } catch (...) {
            continue;
        }
    }
    
    return total;
}

int16_t RecordManager::getSlotCount(const Page& page) const {
    int16_t count;
    std::memcpy(&count, page.data() + PAGE_RECORD_COUNT_OFFSET, sizeof(count));
    return count;
}

int16_t RecordManager::getFreeOffset(const Page& page) const {
    int16_t freeOffset;
    std::memcpy(&freeOffset, page.data() + PAGE_FREE_OFFSET_OFFSET, sizeof(freeOffset));
    return freeOffset;
}

void RecordManager::setFreeOffset(Page& page, int16_t freeOffset) {
    std::memcpy(page.data() + PAGE_FREE_OFFSET_OFFSET, &freeOffset, sizeof(freeOffset));
}

void RecordManager::setRecordCount(Page& page, int16_t recordCount) {
    std::memcpy(page.data() + PAGE_RECORD_COUNT_OFFSET, &recordCount, sizeof(recordCount));
}

RecordManager::Slot RecordManager::getSlot(const Page& page, int16_t slotIndex) const {
    Slot slot;
    const char* slotPtr = page.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE;
    std::memcpy(&slot.offset, slotPtr, 2);
    std::memcpy(&slot.size, slotPtr + 2, 2);
    std::memcpy(&slot.reserved, slotPtr + 4, 4);
    return slot;
}

void RecordManager::setSlot(Page& page, int16_t slotIndex, const Slot& slot) {
    char* slotPtr = page.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE;
    std::memcpy(slotPtr, &slot.offset, 2);
    std::memcpy(slotPtr + 2, &slot.size, 2);
    std::memcpy(slotPtr + 4, &slot.reserved, 4);
}

void RecordManager::removeSlot(Page& page, int16_t slotIndex) {
    Slot slot = getSlot(page, slotIndex);
    slot.size = 0;
    setSlot(page, slotIndex, slot);
}


int16_t RecordManager::findFreeSlotIndex(const Page& page) const {
    int16_t recordCount = getSlotCount(page);
    
    for (int16_t i = 0; i < recordCount; ++i) {
        Slot slot = getSlot(page, i);
        if (slot.size == 0) {
            return i;
        }
    }
    
    return INVALID_SLOT;
}

PageID_t RecordManager::findPageWithSpace(size_t neededBytes) {
    size_t totalNeeded = neededBytes + SLOT_SIZE;
    int32_t pageCount = pm_.pageCount();
    
    for (PageID_t pageID = 1; pageID < pageCount; ++pageID) {
        try {
            Page page = pm_.readPage(pageID);
            int16_t freeOffset = getFreeOffset(page);
            int16_t recordCount = getSlotCount(page);
            
            int16_t slotsEnd = PAGE_HEADER_SIZE + (recordCount + 1) * SLOT_SIZE;
            size_t availableSpace = static_cast<size_t>(freeOffset - slotsEnd);
            
            if (availableSpace >= totalNeeded) {
                return pageID;
            }
        } catch (...) {
            continue;
        }
    }
    
    return pm_.allocatePage();
}

int16_t RecordManager::findInsertPosition(const Page& page, size_t recordSize) const {
    int16_t freeOffset = getFreeOffset(page);
    int16_t newFreeOffset = freeOffset - static_cast<int16_t>(recordSize);
    
    int16_t recordCount = getSlotCount(page);
    int16_t slotsEnd = PAGE_HEADER_SIZE + (recordCount + 1) * SLOT_SIZE;
    
    if (newFreeOffset < slotsEnd) {
        throw std::runtime_error("RecordManager::findInsertPosition: Not enough space");
    }
    
    return newFreeOffset;
}

void RecordManager::appendRecord(Page& page, const std::vector<char>& data, RecordID& recordID) {
    size_t recordSize = data.size();
    
    int16_t slotIndex = findFreeSlotIndex(page);
    if (slotIndex == INVALID_SLOT) {
        slotIndex = getSlotCount(page);
        setRecordCount(page, getSlotCount(page) + 1);
    }
    
    int16_t insertPosition = findInsertPosition(page, recordSize);
    std::memcpy(page.data() + insertPosition, data.data(), recordSize);
    
    Slot slot{insertPosition, static_cast<int16_t>(recordSize), 0};
    setSlot(page, slotIndex, slot);
    setFreeOffset(page, insertPosition);
    
    recordID.slotID = slotIndex;
}

void RecordManager::compactPage(Page& page) {
    int16_t recordCount = getSlotCount(page);
    
    if (recordCount == 0) {
        clearPage(page);
        return;
    }
    
    struct LiveRecord {
        int16_t oldOffset;
        int16_t size;
        int16_t slotIndex;
    };
    
    std::vector<LiveRecord> liveRecords;
    size_t totalDataSize = 0;
    
    for (int16_t i = 0; i < recordCount; ++i) {
        Slot slot = getSlot(page, i);
        if (slot.size > 0) {
            liveRecords.push_back({slot.offset, slot.size, i});
            totalDataSize += slot.size;
        }
    }
    
    if (liveRecords.empty()) {
        clearPage(page);
        return;
    }
    
    int16_t freeOffset = getFreeOffset(page);
    int16_t usedSpace = PAGE_SIZE - freeOffset;
    int16_t wasteSpace = usedSpace - static_cast<int16_t>(totalDataSize);
    
    if (wasteSpace < PAGE_SIZE / 20) {
        return;
    }
    
    Page tempPage{};
    std::memcpy(tempPage.data(), page.data(), PAGE_HEADER_SIZE);
    
    int16_t currentOffset = PAGE_SIZE;
    for (size_t i = liveRecords.size(); i > 0; --i) {
        currentOffset -= liveRecords[i - 1].size;
        
        std::memcpy(tempPage.data() + currentOffset,
                    page.data() + liveRecords[i - 1].oldOffset,
                    liveRecords[i - 1].size);
        
        liveRecords[i - 1].oldOffset = currentOffset;
    }
    
    for (const auto& rec : liveRecords) {
        char* slotPtr = tempPage.data() + PAGE_HEADER_SIZE + rec.slotIndex * SLOT_SIZE;
        std::memcpy(slotPtr, &rec.oldOffset, 2);
        std::memcpy(slotPtr + 2, &rec.size, 2);
        int32_t reserved = 0;
        std::memcpy(slotPtr + 4, &reserved, 4);
    }
    
    int16_t newRecordCount = static_cast<int16_t>(liveRecords.size());
    std::memcpy(tempPage.data() + PAGE_RECORD_COUNT_OFFSET, &newRecordCount, 2);
    std::memcpy(tempPage.data() + PAGE_FREE_OFFSET_OFFSET, &currentOffset, 2);
    
    std::memcpy(page.data(), tempPage.data(), PAGE_SIZE);
}

bool RecordManager::shouldCompact(const Page& page) const {
    int16_t recordCount = getSlotCount(page);
    if (recordCount == 0) return false;
    
    int16_t freeOffset = getFreeOffset(page);
    int16_t usedSpace = PAGE_SIZE - freeOffset;
    
    int16_t activeData = 0;
    for (int16_t i = 0; i < recordCount; ++i) {
        Slot slot = getSlot(page, i);
        if (slot.size > 0) {
            activeData += slot.size;
        }
    }
    
    int16_t wastedSpace = usedSpace - activeData;
    return wastedSpace > usedSpace / 3;
}

bool RecordManager::isPageEmpty(const Page& page) const {
    int16_t recordCount = getSlotCount(page);
    
    for (int16_t i = 0; i < recordCount; ++i) {
        Slot slot = getSlot(page, i);
        if (slot.size > 0) {
            return false;
        }
    }
    
    return true;
}

void RecordManager::clearPage(Page& page) {
    PageID_t nextPage = -1;
    int16_t recordCount = 0;
    int16_t freeOffset = PAGE_SIZE;
    int32_t flags = 0;
    
    std::memcpy(page.data() + PAGE_NEXT_PAGE_OFFSET, &nextPage, sizeof(nextPage));
    std::memcpy(page.data() + PAGE_RECORD_COUNT_OFFSET, &recordCount, 2);
    std::memcpy(page.data() + PAGE_FREE_OFFSET_OFFSET, &freeOffset, 2);
    std::memcpy(page.data() + PAGE_FLAGS_OFFSET, &flags, sizeof(flags));
    
    std::memset(page.data() + PAGE_HEADER_SIZE, 0, PAGE_SIZE - PAGE_HEADER_SIZE);
}
