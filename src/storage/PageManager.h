#ifndef DBMS_PAIN_PAGEMANAGER_H
#define DBMS_PAIN_PAGEMANAGER_H

#include <array>
#include <cstdint>
#include <string>

constexpr int PAGE_SIZE = 4096;
using Page = std::array<char, PAGE_SIZE>;
using page_id_t = int32_t;

class PageManager {
public:
  explicit PageManager(const std::string &file_path);
  ~PageManager();

  // Прочитать страницу с диска в память
  Page read_page(page_id_t page_id);

  // Записать страницу из памяти на диск
  void write_page(page_id_t page_id, const Page &page);

  // Выделить новую страницу (расширить файл)
  page_id_t allocate_page();

  // Пометить страницу как свободную
  void free_page(page_id_t page_id);

  // Сколько страниц в файле
  int32_t page_count() const;

private:
  std::string file_path_;
  std::fstream file_;
  int32_t page_count_;

  void init_file();   // создать файл если не существует
  void load_header(); // прочитать page_count из заголовка
  void save_header(); // сохранить page_count в заголовок
};

#endif // DBMS_PAIN_PAGEMANAGER_H