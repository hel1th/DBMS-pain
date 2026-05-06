#include "RecordManager.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace {

// Константы страницы
constexpr int16_t PAGE_HEADER_SIZE = 16;
constexpr int16_t PAGE_NEXT_PAGE_OFFSET = 0;      // 4 байта (PageID_t)
constexpr int16_t PAGE_RECORD_COUNT_OFFSET = 4;   // 2 байта (int16_t)
constexpr int16_t PAGE_FREE_OFFSET_OFFSET = 6;    // 2 байта (int16_t)
constexpr int16_t PAGE_FLAGS_OFFSET = 8;          // 4 байта (int32_t)

constexpr int16_t SLOT_SIZE = 8;  // offset(2) + size(2) + reserved(4)
constexpr int16_t INVALID_SLOT = -1;

// Порог фрагментации: компактифицировать если > 30% места занято удалёнными записями
constexpr double COMPACT_THRESHOLD = 0.3;

} // anonymous namespace

RecordManager::RecordManager(PageManager& pm, const Schema& schema)
    : pm_(pm), schema_(schema) {}

// ========== Публичные методы ==========

RecordID RecordManager::Insert(const std::vector<Value>& record) {
    // 1. Сериализуем запись
    std::vector<char> data = Serializer::Serialize(record, schema_);
    size_t recordSize = data.size();
    
    // 2. Находим страницу с местом
    PageID_t pageID = FindPageWithSpace(recordSize);
    
    // 3. Читаем страницу
    Page page = pm_.ReadPage(pageID);
    
    // 4. Вставляем запись
    RecordID recordID;
    recordID.pageID = pageID;
    
    // Находим свободный слот или создаём новый
    int16_t slotIndex = FindFreeSlotIndex(page);
    if (slotIndex == -1) {
        // Новый слот в конец
        int16_t recordCount = GetSlotCount(page);
        slotIndex = recordCount;
        SetRecordCount(page, recordCount + 1);
    }
    
    // Вычисляем смещение для записи (данные растут с конца страницы)
    int16_t freeOffset = GetFreeOffset(page);
    int16_t newFreeOffset = freeOffset - static_cast<int16_t>(recordSize);
    
    // Проверяем, что не перекрываемся со слотами
    int16_t slotsEnd = PAGE_HEADER_SIZE + (GetSlotCount(page) + 1) * SLOT_SIZE;
    if (newFreeOffset < slotsEnd) {
        // Недостаточно места — компактифицируем и пробуем снова
        CompactPage(page);
        freeOffset = GetFreeOffset(page);
        newFreeOffset = freeOffset - static_cast<int16_t>(recordSize);
        
        if (newFreeOffset < PAGE_HEADER_SIZE + (GetSlotCount(page) + 1) * SLOT_SIZE) {
            throw std::runtime_error("Record too large even after compaction");
        }
    }
    
    std::memcpy(page.data() + newFreeOffset, data.data(), recordSize);
    
    Slot slot;
    slot.offset = newFreeOffset;
    slot.size = static_cast<int16_t>(recordSize);
    slot.reserved = 0;
    SetSlot(page, slotIndex, slot);
    
    SetFreeOffset(page, newFreeOffset);
    
    pm_.WritePage(pageID, page);
    
    recordID.slotID = slotIndex;
    return recordID;
}

std::vector<Value> RecordManager::Fetch(RecordID recordID) {
    // 1. Проверка валидности
    if (!IsValid(recordID)) {
        throw std::runtime_error("Invalid RecordID");
    }
    
    // 2. Читаем страницу
    Page page = pm_.ReadPage(recordID.pageID);
    
    // 3. Читаем слот
    Slot slot = GetSlot(page, recordID.slotID);
    
    if (slot.size == 0) {
        throw std::runtime_error("Record has been deleted");
    }
    
    // 4. Извлекаем данные
    const char* dataPtr = page.data() + slot.offset;
    
    // 5. Десериализуем
    return Serializer::Deserialize(dataPtr, static_cast<size_t>(slot.size), schema_);
}

void RecordManager::Update(RecordID recordID, const std::vector<Value>& record) {
    if (!IsValid(recordID)) {
        throw std::runtime_error("Invalid RecordID");
    }
    
    std::vector<char> newData = Serializer::Serialize(record, schema_);
    size_t newSize = newData.size();
    
    Page page = pm_.ReadPage(recordID.pageID);
    Slot oldSlot = GetSlot(page, recordID.slotID);
    
    if (oldSlot.size == 0) {
        throw std::runtime_error("Cannot update deleted record");
    }
    
    if (static_cast<size_t>(oldSlot.size) == newSize) {
        std::memcpy(page.data() + oldSlot.offset, newData.data(), newSize);
        pm_.WritePage(recordID.pageID, page);
        return;
    }
    
    Remove(recordID);
    RecordID newRid = Insert(record);
    
    if (newRid.pageID != recordID.pageID || newRid.slotID != recordID.slotID) {
        // Если запись переместилась, нужно обновить индексы
        // TODO: Уведомить IndexManager о перемещении
    }
}

void RecordManager::Remove(RecordID recordID) {
    if (!IsValid(recordID)) {
        throw std::runtime_error("Invalid RecordID");
    }
    
    // 1. Читаем страницу
    Page page = pm_.ReadPage(recordID.pageID);
    
    // 2. Помечаем слот как удалённый (size = 0)
    RemoveSlot(page, recordID.slotID);
    
    // 3. Сохраняем страницу
    pm_.WritePage(recordID.pageID, page);
    
    // 4. Если страница пуста, освобождаем её
    if (IsPageEmpty(page)) {
        pm_.FreePage(recordID.pageID);
    }
}

void RecordManager::Scan(std::function<void(RecordID, const std::vector<Value>&)> callback) {
    int32_t pageCount = pm_.PageCount();
    
    for (PageID_t pageID = 1; pageID < pageCount; pageID++) {  // Page 0 — заголовочная
        try {
            Page page = pm_.ReadPage(pageID);
            int16_t recordCount = GetSlotCount(page);
            
            for (int16_t slotId = 0; slotId < recordCount; slotId++) {
                Slot slot = GetSlot(page, slotId);
                if (slot.size > 0) {
                    RecordID recordID{pageID, slotId};
                    try {
                        std::vector<Value> values = Fetch(recordID);
                        callback(recordID, values);
                    } catch (const std::exception& e) {
                        // Логируем ошибку, но продолжаем сканирование
                        continue;
                    }
                }
            }
        } catch (const std::exception& e) {
            continue;
        }
    }
}


int16_t RecordManager::GetSlotCount(const Page& page) const {
    int16_t count;
    std::memcpy(&count, page.data() + PAGE_RECORD_COUNT_OFFSET, sizeof(count));
    return count;
}

int16_t RecordManager::GetFreeOffset(const Page& page) const {
    int16_t freeOffset;
    std::memcpy(&freeOffset, page.data() + PAGE_FREE_OFFSET_OFFSET, sizeof(freeOffset));
    return freeOffset;
}

void RecordManager::SetFreeOffset(Page& page, int16_t freeOffset) {
    std::memcpy(page.data() + PAGE_FREE_OFFSET_OFFSET, &freeOffset, sizeof(freeOffset));
}

void RecordManager::SetRecordCount(Page& page, int16_t recordCount) {
    std::memcpy(page.data() + PAGE_RECORD_COUNT_OFFSET, &recordCount, sizeof(recordCount));
}

RecordManager::Slot RecordManager::GetSlot(const Page& page, int16_t slotIndex) const {
    Slot slot;
    const char* slotPtr = page.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE;
    std::memcpy(&slot.offset, slotPtr, 2);
    std::memcpy(&slot.size, slotPtr + 2, 2);
    std::memcpy(&slot.reserved, slotPtr + 4, 4);
    return slot;
}

void RecordManager::SetSlot(Page& page, int16_t slotIndex, const Slot& slot) {
    char* slotPtr = page.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE;
    std::memcpy(slotPtr, &slot.offset, 2);
    std::memcpy(slotPtr + 2, &slot.size, 2);
    std::memcpy(slotPtr + 4, &slot.reserved, 4);
}

void RecordManager::RemoveSlot(Page& page, int16_t slotIndex) {
    Slot slot = GetSlot(page, slotIndex);
    slot.size = 0;
    SetSlot(page, slotIndex, slot);
}

int16_t RecordManager::FindFreeSlotIndex(const Page& page) const {
    int16_t recordCount = GetSlotCount(page);
    for (int16_t i = 0; i < recordCount; i++) {
        Slot slot = GetSlot(page, i);
        if (slot.size == 0) {
            return i;  // Нашли удалённый слот
        }
    }
    return INVALID_SLOT;  // Нет свободных слотов
}

PageID_t RecordManager::FindPageWithSpace(size_t neededBytes) {
    // Размер данных + слот
    size_t totalNeeded = neededBytes + SLOT_SIZE;
    
    int32_t pageCount = pm_.PageCount();
    
    // Ищем существующую страницу с местом
    for (PageID_t pageID = 1; pageID < pageCount; pageID++) {
        try {
            Page page = pm_.ReadPage(pageID);
            int16_t freeOffset = GetFreeOffset(page);
            int16_t recordCount = GetSlotCount(page);
            int16_t slotsEnd = PAGE_HEADER_SIZE + (recordCount + 1) * SLOT_SIZE;
            size_t availableSpace = static_cast<size_t>(freeOffset - slotsEnd);
            
            if (availableSpace >= totalNeeded) {
                return pageID;
            }
        } catch (...) {
            // Пропускаем повреждённые страницы
            continue;
        }
    }
    
    // Нет подходящей страницы — создаём новую
    return pm_.AllocatePage();
}

void RecordManager::CompactPage(Page& page) {
    int16_t recordCount = GetSlotCount(page);
    
    if (recordCount == 0) {
        return;
    }
    
    // 1. Собираем живые записи
    std::vector<Slot> liveSlots;
    std::vector<size_t> liveIndices;
    size_t totalDataSize = 0;
    
    for (int16_t i = 0; i < recordCount; i++) {
        Slot slot = GetSlot(page, i);
        if (slot.size > 0) {
            liveSlots.push_back(slot);
            liveIndices.push_back(i);
            totalDataSize += slot.size;
        }
    }
    
    if (liveSlots.empty()) {
        // Нет живых записей — очищаем страницу
        ClearPage(page);
        return;
    }
    
    // 2. Проверяем, нужна ли компактификация
    int16_t freeOffset = GetFreeOffset(page);
    int16_t usedSpace = PAGE_SIZE - freeOffset;
    
    // Если фрагментация небольшая, не компактифицируем
    if (static_cast<size_t>(usedSpace - totalDataSize) < PAGE_SIZE / 10) {
        return;  // Маленькая фрагментация, не трогаем
    }
    
    // 3. Создаём временный буфер для данных
    //    ВНИМАНИЕ: это массив char, а не vector!
    char tempBuffer[PAGE_SIZE];
    
    // 4. Копируем заголовок
    std::memcpy(tempBuffer, page.data(), PAGE_HEADER_SIZE);
    
    // 5. Копируем живые записи в конец временного буфера
    int16_t currentOffset = PAGE_SIZE;
    for (size_t i = liveSlots.size(); i > 0; i--) {
        const Slot& slot = liveSlots[i - 1];
        currentOffset -= slot.size;
        
        // Копируем данные
        std::memcpy(tempBuffer + currentOffset, 
                    page.data() + slot.offset, 
                    slot.size);
        
        // Обновляем слот
        liveSlots[i - 1].offset = currentOffset;
    }
    
    // 6. Записываем новые слоты
    for (size_t i = 0; i < liveSlots.size(); i++) {
        char* slotPtr = tempBuffer + PAGE_HEADER_SIZE + i * SLOT_SIZE;
        std::memcpy(slotPtr, &liveSlots[i].offset, 2);
        std::memcpy(slotPtr + 2, &liveSlots[i].size, 2);
        std::memcpy(slotPtr + 4, &liveSlots[i].reserved, 4);
    }
    
    // 7. Обновляем заголовок
    int16_t newRecordCount = static_cast<int16_t>(liveSlots.size());
    std::memcpy(tempBuffer + PAGE_RECORD_COUNT_OFFSET, &newRecordCount, 2);
    std::memcpy(tempBuffer + PAGE_FREE_OFFSET_OFFSET, &currentOffset, 2);
    
    // 8. Копируем обратно на страницу
    std::memcpy(page.data(), tempBuffer, PAGE_SIZE);
}
bool RecordManager::ShouldCompact(const Page& page) const {
    int16_t recordCount = GetSlotCount(page);
    if (recordCount == 0) return false;
    
    int16_t freeOffset = GetFreeOffset(page);
    int16_t usedSpace = PAGE_SIZE - freeOffset;
    
    // Считаем активные данные
    int16_t activeData = 0;
    for (int16_t i = 0; i < recordCount; i++) {
        Slot slot = GetSlot(page, i);
        if (slot.size > 0) {
            activeData += slot.size;
        }
    }
    
    // Если удалённые данные занимают больше 30% использованного пространства
    int16_t wastedSpace = usedSpace - activeData;
    return (wastedSpace > usedSpace / 3);
}

bool RecordManager::IsPageEmpty(const Page& page) const {
    int16_t recordCount = GetSlotCount(page);
    
    for (int16_t i = 0; i < recordCount; i++) {
        Slot slot = GetSlot(page, i);
        if (slot.size > 0) {
            return false;  // Есть живая запись
        }
    }
    
    return true;
}

void RecordManager::ClearPage(Page& page) {
    // Очищаем заголовок
    int32_t nextPage = -1;
    int16_t recordCount = 0;
    int16_t freeOffset = PAGE_SIZE;
    int32_t flags = 0;
    
    std::memcpy(page.data() + PAGE_NEXT_PAGE_OFFSET, &nextPage, 4);
    std::memcpy(page.data() + PAGE_RECORD_COUNT_OFFSET, &recordCount, 2);
    std::memcpy(page.data() + PAGE_FREE_OFFSET_OFFSET, &freeOffset, 2);
    std::memcpy(page.data() + PAGE_FLAGS_OFFSET, &flags, 4);
    
    // Остальное заполняем нулями
    std::memset(page.data() + PAGE_HEADER_SIZE, 0, PAGE_SIZE - PAGE_HEADER_SIZE);
}

bool RecordManager::IsValid(RecordID recordID) const {
    if (recordID.pageID <= 0 || recordID.pageID >= pm_.PageCount()) {
        return false;
    }
    
    if (recordID.slotID < 0) {
        return false;
    }
    
    try {
        Page page = pm_.ReadPage(recordID.pageID);
        int16_t recordCount = GetSlotCount(page);
        
        if (recordID.slotID >= recordCount) {
            return false;
        }
        
        Slot slot = GetSlot(page, recordID.slotID);
        return slot.size > 0;
    } catch (...) {
        return false;
    }
}

size_t RecordManager::GetRecordCount() const {
    size_t total = 0;
    int32_t pageCount = pm_.PageCount();
    
    for (PageID_t pageID = 1; pageID < pageCount; pageID++) {
        try {
            Page page = pm_.ReadPage(pageID);
            total += static_cast<size_t>(GetSlotCount(page));
        } catch (...) {
            continue;
        }
    }
    
    return total;
}