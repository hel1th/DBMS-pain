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
constexpr int PAGE_RECORD_COUNT_OFFSET = 4;  // 4-5: record_count
constexpr int PAGE_FREE_OFFSET_OFFSET = 6;   // 6-7: free_offset
constexpr int PAGE_FLAGS_OFFSET = 8;         // 8-11: flags
// 12-15: зарезервировано

constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;

PageManager::PageManager(const std::string &file_path) 
    : file_path_(file_path), page_count_(0), free_list_head_(-1) {
    
    // Проверяем существование файла
    struct stat buffer;
    bool file_exists = (stat(file_path_.c_str(), &buffer) == 0);
    
    // Открываем файл для чтения и записи в бинарном режиме
    file_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);
    
    if (!file_exists) {
        // Файл не существует - создаём новый
        init_file();
    } else {
        // Файл существует - загружаем заголовок
        load_header();
    }
}

PageManager::~PageManager() {
    if (file_.is_open()) {
        save_header();
        file_.close();
    }
}

void PageManager::init_file() {
    // Закрываем и создаём новый файл
    if (file_.is_open()) {
        file_.close();
    }
    
    file_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot create file: " + file_path_);
    }
    
    // Создаём заголовочную страницу (Page 0)
    Page header_page{};
    
    // Записываем magic number
    uint32_t magic = MAGIC_NUMBER;
    std::memcpy(header_page.data() + HEADER_MAGIC_OFFSET, &magic, sizeof(magic));
    
    // Изначально 1 страница (только заголовочная)
    page_count_ = 1;
    std::memcpy(header_page.data() + HEADER_PAGE_COUNT_OFFSET, &page_count_, sizeof(page_count_));
    
    // Список свободных страниц пуст (-1 означает конец списка)
    free_list_head_ = -1;
    std::memcpy(header_page.data() + HEADER_FREE_LIST_OFFSET, &free_list_head_, sizeof(free_list_head_));
    
    // Количество записей = 0
    int32_t record_count = 0;
    std::memcpy(header_page.data() + HEADER_RECORD_COUNT_OFFSET, &record_count, sizeof(record_count));
    
    // Остальная часть страницы (16+ байт) уже обнулена благодаря {} инициализации
    
    // Записываем заголовочную страницу
    file_.seekp(0, std::ios::beg);
    file_.write(header_page.data(), PAGE_SIZE);
    file_.flush();
}

void PageManager::load_header() {
    // Перемещаемся в начало файла
    file_.seekg(0, std::ios::beg);
    
    // Читаем заголовочную страницу
    Page header_page{};
    file_.read(header_page.data(), PAGE_SIZE);
    
    if (!file_.good()) {
        throw std::runtime_error("Failed to read header page");
    }
    
    // Проверяем magic number
    uint32_t magic;
    std::memcpy(&magic, header_page.data() + HEADER_MAGIC_OFFSET, sizeof(magic));
    
    if (magic != MAGIC_NUMBER) {
        throw std::runtime_error("Invalid file format: wrong magic number");
    }
    
    // Загружаем количество страниц
    std::memcpy(&page_count_, header_page.data() + HEADER_PAGE_COUNT_OFFSET, sizeof(page_count_));
    std::memcpy(&free_list_head_, header_page.data() + HEADER_FREE_LIST_OFFSET, sizeof(free_list_head_));
}

void PageManager::save_header() {
    // Читаем текущую заголовочную страницу
    Page header_page{};
    file_.seekg(0, std::ios::beg);
    file_.read(header_page.data(), PAGE_SIZE);
    
    if (!file_.good()) {
        // Если не удалось прочитать, создаём новую
        header_page = Page{};
        uint32_t magic = MAGIC_NUMBER;
        std::memcpy(header_page.data() + HEADER_MAGIC_OFFSET, &magic, sizeof(magic));
    }
    
    std::memcpy(header_page.data() + HEADER_PAGE_COUNT_OFFSET, &page_count_, sizeof(page_count_));
    std::memcpy(header_page.data() + HEADER_FREE_LIST_OFFSET, &free_list_head_, sizeof(free_list_head_));
    
    // Записываем обратно
    file_.seekp(0, std::ios::beg);
    file_.write(header_page.data(), PAGE_SIZE);
    file_.flush();
}

Page PageManager::read_page(page_id_t page_id) {
    if (page_id < 0 || page_id >= page_count_) {
        throw std::out_of_range("Invalid page_id: " + std::to_string(page_id) + 
                               ". Page count: " + std::to_string(page_count_));
    }
    
    Page page{};
    
    // Вычисляем смещение в файле
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    
    file_.seekg(offset, std::ios::beg);
    file_.read(page.data(), PAGE_SIZE);
    
    if (!file_.good()) {
        throw std::runtime_error("Failed to read page " + std::to_string(page_id));
    }
    
    return page;
}

void PageManager::write_page(page_id_t page_id, const Page &page) {
    if (page_id < 0 || page_id >= page_count_) {
        throw std::out_of_range("Invalid page_id: " + std::to_string(page_id) + 
                               ". Page count: " + std::to_string(page_count_));
    }
    
    // Вычисляем смещение в файле
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    
    file_.seekp(offset, std::ios::beg);
    file_.write(page.data(), PAGE_SIZE);
    file_.flush();
    
    if (!file_.good()) {
        throw std::runtime_error("Failed to write page " + std::to_string(page_id));
    }
}

page_id_t PageManager::allocate_page() {
    page_id_t new_page_id;
    
    if (free_list_head_ != -1) {
        // Есть свободные страницы
        new_page_id = free_list_head_;
        
        // Читаем эту страницу, чтобы получить next_page
        Page free_page = read_page(new_page_id);
        
        // Обновляем free_list_head_ на следующую свободную страницу
        std::memcpy(&free_list_head_, free_page.data() + PAGE_NEXT_PAGE_OFFSET, sizeof(free_list_head_));
        
        // Сохраняем обновлённый заголовок
        save_header();
        
        // Очищаем страницу перед использованием
        Page empty_page{};
        write_page(new_page_id, empty_page);
    } else {
        // Нет свободных страниц - расширяем файл
        new_page_id = page_count_;
        page_count_++;
        
        // Записываем пустую страницу в конец файла
        Page empty_page{};
        file_.seekp(0, std::ios::end);
        file_.write(empty_page.data(), PAGE_SIZE);
        file_.flush();
        
        // Сохраняем обновлённый page_count
        save_header();
    }
    
    return new_page_id;
}

void PageManager::free_page(page_id_t page_id) {
    // Нельзя освобождать заголовочную страницу
    if (page_id == 0) {
        throw std::runtime_error("Cannot free header page (page 0)");
    }
    
    if (page_id < 0 || page_id >= page_count_) {
        throw std::out_of_range("Invalid page_id for free: " + std::to_string(page_id));
    }
    
    if (page_id == free_list_head_) {
        throw std::runtime_error("Page already in free list");
    }
    
    // Можно также пройти по всему списку и проверить (но медленно)
    
    Page free_page = read_page(page_id);
    
    // Устанавливаем next_page этой страницы на текущую голову списка
    std::memcpy(free_page.data() + PAGE_NEXT_PAGE_OFFSET, &free_list_head_, sizeof(free_list_head_));
    
    // Записываем обновлённую страницу
    write_page(page_id, free_page);
    
    // Обновляем голову списка
    free_list_head_ = page_id;
    
    // Сохраняем заголовок
    save_header();
}

int32_t PageManager::page_count() const {
    return page_count_;
}