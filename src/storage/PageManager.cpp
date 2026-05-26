#include "PageManager.h"

#include <cstring>
#include <stdexcept>
#include <sys/stat.h>

// Константы для заголовочной страницы (Page 0)
constexpr int HEADER_MAGIC_OFFSET = 0;      // 0-3: magic number
constexpr int HEADER_PAGE_COUNT_OFFSET = 4; // 4-7: количество страниц
constexpr int HEADER_FREE_LIST_OFFSET = 8;  // 8-11: первая свободная страница
constexpr int HEADER_RECORD_COUNT_OFFSET = 12; // 12-15: количество записей

// Константы для обычной страницы
constexpr int PAGE_NEXT_PAGE_OFFSET = 0;     // 0-3: next_page
constexpr int PAGE_RECORD_COUNT_OFFSET = 4;  // 4-5: recordCount
constexpr int PAGE_FREE_OFFSET_OFFSET = 6;   // 6-7: free_offset
constexpr int PAGE_FLAGS_OFFSET = 8;         // 8-11: flags
// 12-15: зарезервировано

constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;

PageManager::PageManager(const std::string &filePath) 
    : filePath_(filePath), pageCount_(0), freeListHead_(-1) {
    
    // Проверяем существование файла
    struct stat buffer;
    bool fileExists = (stat(filePath_.c_str(), &buffer) == 0);
    
    file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary);

    if (!fileExists) {
        // Если файл не существует, создаём
        if (!file_.is_open()) {
            // Файл не открылся, создаем с trunc
            file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        }
        initFile();
    } else {
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath_);
        }
        loadHeader();
    }
}

PageManager::~PageManager() {
    if (file_.is_open()) {
        saveHeader();
        file_.close();
    }
}

void PageManager::initFile() {
    if (file_.is_open()) {
        file_.close();
    }
    
    file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot create file: " + filePath_);
    }
    
    Page headerPage{};
    
    uint32_t magic = MAGIC_NUMBER;
    std::memcpy(headerPage.data() + HEADER_MAGIC_OFFSET, &magic, sizeof(magic));
    
    // Изначально 1 страница (заголовок)
    pageCount_ = 1;
    std::memcpy(headerPage.data() + HEADER_PAGE_COUNT_OFFSET, &pageCount_, sizeof(pageCount_));
    
    // Список свободных страниц пуст
    freeListHead_ = -1;
    std::memcpy(headerPage.data() + HEADER_FREE_LIST_OFFSET, &freeListHead_, sizeof(freeListHead_));
    
    // Колво записей 0
    int32_t recordCount = 0;
    std::memcpy(headerPage.data() + HEADER_RECORD_COUNT_OFFSET, &recordCount, sizeof(recordCount));
    
    // Записываем заголовочную страницу
    file_.seekp(0, std::ios::beg);
    file_.write(headerPage.data(), PAGE_SIZE);
    file_.flush();
}

void PageManager::loadHeader() {
    // Перемещаемся в начало файла
    file_.seekg(0, std::ios::beg);
    
    // Читаем заголовочную страницу
    Page headerPage{};
    file_.read(headerPage.data(), PAGE_SIZE);
    
    if (!file_.good()) {
        throw std::runtime_error("Failed to read header page");
    }
    
    // Проверяем magic number
    uint32_t magic;
    std::memcpy(&magic, headerPage.data() + HEADER_MAGIC_OFFSET, sizeof(magic));
    
    if (magic != MAGIC_NUMBER) {
        throw std::runtime_error("Invalid file format: wrong magic number");
    }
    
    // Загружаем количество страниц
    std::memcpy(&pageCount_, headerPage.data() + HEADER_PAGE_COUNT_OFFSET, sizeof(pageCount_));
    std::memcpy(&freeListHead_, headerPage.data() + HEADER_FREE_LIST_OFFSET, sizeof(freeListHead_));
}

void PageManager::saveHeader() {
    // Читаем текущий заголовок
    Page headerPage{};
    file_.seekg(0, std::ios::beg);
    file_.read(headerPage.data(), PAGE_SIZE);
    
    if (!file_.good()) {
        // не удалось прочитать - создаём новую
        headerPage = Page{};
    }
    
    uint32_t magic = MAGIC_NUMBER;
    std::memcpy(headerPage.data() + HEADER_MAGIC_OFFSET, &magic, sizeof(magic));
    std::memcpy(headerPage.data() + HEADER_PAGE_COUNT_OFFSET, &pageCount_, sizeof(pageCount_));
    std::memcpy(headerPage.data() + HEADER_FREE_LIST_OFFSET, &freeListHead_, sizeof(freeListHead_));
    
    file_.seekp(0, std::ios::beg);
    file_.write(headerPage.data(), PAGE_SIZE);
    file_.flush();
}

Page PageManager::readPage(PageID_t pageID) {
    if (pageID < 0 || pageID >= pageCount_) {
        throw std::out_of_range("Invalid page_id: " + std::to_string(pageID) + 
                               ". Page count: " + std::to_string(pageCount_));
    }
    
    Page page{};
    
    // Вычисляем смещение в файле
    std::streamoff offset = static_cast<std::streamoff>(pageID) * PAGE_SIZE;
    
    file_.seekg(offset, std::ios::beg);
    file_.read(page.data(), PAGE_SIZE);

    if (file_.gcount() != PAGE_SIZE) {
        throw std::runtime_error("Failed to read full page " + std::to_string(pageID) +
                                 ": read " + std::to_string(file_.gcount()) + " bytes");
    }

    if (!file_.good()) {
        throw std::runtime_error("Failed to read page " + std::to_string(pageID));
    }
    
    return page;
}

void PageManager::writePage(PageID_t pageID, const Page &page) {
    if (pageID < 0 || pageID >= pageCount_) {
        throw std::out_of_range("Invalid pageID: " + std::to_string(pageID) + 
                               ". Page count: " + std::to_string(pageCount_));
    }
    
    // Вычисляем смещение в файле
    std::streamoff offset = static_cast<std::streamoff>(pageID) * PAGE_SIZE;
    
    file_.seekp(offset, std::ios::beg);
    file_.write(page.data(), PAGE_SIZE);
    file_.flush();
    
    if (!file_.good()) {
        throw std::runtime_error("Failed to write page " + std::to_string(pageID));
    }
}

PageID_t PageManager::allocatePage() {
    PageID_t newPageId;
    
    if (freeListHead_ != -1) {
        // Есть свободные страницы
        newPageId = freeListHead_;
        
        // Читаем эту страницу, чтобы получить nextPage
        Page freePage = readPage(newPageId);
        
        // Обновляем free_list_head_ на следующую свободную страницу
        std::memcpy(&freeListHead_, freePage.data() + PAGE_NEXT_PAGE_OFFSET, sizeof(freeListHead_));
        saveHeader();
        
        // Очищаем страницу перед использованием
        Page emptyPage{};
        writePage(newPageId, emptyPage);
    } else {
        // Нет свободных страниц - расширяем файл
        newPageId = pageCount_;
        pageCount_++;
        
        // Записываем пустую страницу в конец файла
        Page emptyPage{};
        file_.seekp(0, std::ios::end);
        file_.write(emptyPage.data(), PAGE_SIZE);
        file_.flush();

        saveHeader();
    }
    
    return newPageId;
}

void PageManager::freePage(PageID_t pageID) {
    if (pageID == 0) {
        throw std::runtime_error("Cannot free header page (page 0)");
    }

    if (pageID < 0 || pageID >= pageCount_) {
        throw std::out_of_range("Invalid page_id for free: " + std::to_string(pageID));
    }
    
    if (pageID == freeListHead_) {
        throw std::runtime_error("Page already in free list");
    }
    
    Page freePage = readPage(pageID);
    std::memcpy(freePage.data() + PAGE_NEXT_PAGE_OFFSET, &freeListHead_, sizeof(freeListHead_));
    writePage(pageID, freePage);
    freeListHead_ = pageID;
    saveHeader();
}

int32_t PageManager::pageCount() const {
    return pageCount_;
}
