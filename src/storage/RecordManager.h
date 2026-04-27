#ifndef DBMS_PAIN_RECORDMANAGER_H
#define DBMS_PAIN_RECORDMANAGER_H

#include <functional>

#include "PageManager.h"
#include "Serializer.h"
#include "engine/Schema.h"
#include "utils/Value.h"

// Идентификатор записи = номер страницы + номер слота на странице
struct RecordId {
    PageID_t pageID;
    int16_t   slotID;

    bool operator==(const RecordId& o) const {
        return pageID == o.pageID && slotID == o.slotID;
    }
};

class RecordManager {
public:
    RecordManager(PageManager& pm, const Schema& schema);

    // Вставить запись, вернуть её RecordId
    RecordId Insert(const std::vector<Value>& record);

    // Прочитать запись по RecordId
    std::vector<Value> Fetch(RecordId rid);

    // Обновить запись
    void Update(RecordId rid, const std::vector<Value>& record);

    // Удалить запись (пометить слот как свободный)
    void Remove(RecordId rid);

    // Итерация по всем записям (для full scan)
    // Вызывает callback для каждой живой записи
    void Scan(std::function<void(RecordId, const std::vector<Value>&)> cb);

private:
    PageManager& pm_;
    const Schema& schema_;

    // Найти страницу с местом для записи нужного размера
    PageID_t FindPageWithSpace(size_t needed_bytes);

    // Работа со слотами внутри страницы
    int16_t  GetSlotCount(const Page& page);
    int16_t  GetFreeOffset(const Page& page);
    void     WriteSlot(Page& page, int16_t slot, int16_t offset, int16_t size);
    int16_t  GetSlotOffset(const Page& page, int16_t slot);
    int16_t  GetSlotSize(const Page& page, int16_t slot);
};


#endif //DBMS_PAIN_RECORDMANAGER_H