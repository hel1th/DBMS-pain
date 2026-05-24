#ifndef DBMS_PAIN_RECORDMANAGER_H
#define DBMS_PAIN_RECORDMANAGER_H

#include <functional>

#include "PageManager.h"
#include "Serializer.h"
#include "../engine/Schema.h"
#include "../utils/Value.h"

// Идентификатор записи = номер страницы + номер слота на странице
struct RecordID {
    page_id_t pageID;
    int16_t   slotID;

    bool operator==(const RecordID& o) const {
        return pageID == o.pageID && slotID == o.slotID;
    }
};

class RecordManager {
public:
    RecordManager(PageManager& pm, const Schema& schema);

    // Вставить запись, вернуть её RecordId
    RecordID insert(const std::vector<Value>& record);

    // Прочитать запись по RecordId
    std::vector<Value> fetch(RecordID rid);

    // Обновить запись
    void update(RecordID rid, const std::vector<Value>& record);

    // Удалить запись (пометить слот как свободный)
    void remove(RecordID rid);

    // Итерация по всем записям (для full scan)
    // Вызывает callback для каждой живой записи
    void scan(std::function<void(RecordID, const std::vector<Value>&)> cb);

private:
    PageManager& pm_;
    const Schema& schema_;

    // Найти страницу с местом для записи нужного размера
    page_id_t find_page_with_space(size_t needed_bytes);

    // Работа со слотами внутри страницы
    int16_t  get_slot_count(const Page& page);
    int16_t  get_free_offset(const Page& page);
    void     write_slot(Page& page, int16_t slot, int16_t offset, int16_t size);
    int16_t  get_slot_offset(const Page& page, int16_t slot);
    int16_t  get_slot_size(const Page& page, int16_t slot);
};


#endif //DBMS_PAIN_RECORDMANAGER_H