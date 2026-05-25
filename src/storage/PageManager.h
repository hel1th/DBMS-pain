#ifndef DBMS_PAIN_PAGEMANAGER_H
#define DBMS_PAIN_PAGEMANAGER_H

#include <array>
#include <cstdint>
#include <string>
#include <fstream>

constexpr int PAGE_SIZE = 4096;
using Page = std::array<char, PAGE_SIZE>;
using PageID_t = int32_t;

class PageManager {
public:
  explicit PageManager(const std::string &filePath);
  ~PageManager();

  // Прочитать страницу с диска в память
  Page readPage(PageID_t pageID);

  // Записать страницу из памяти на диск
  void writePage(PageID_t pageID, const Page &page);

  // Выделить новую страницу (расширить файл)
  PageID_t allocatePage();

  // Пометить страницу как свободную
  void freePage(PageID_t pageID);

  // Сколько страниц в файле
  int32_t pageCount() const;

private:
  std::string filePath_;
  std::fstream file_;
  int32_t pageCount_;
  int32_t freeListHead_;

  void initFile();   // создать файл если не существует
  void loadHeader(); // прочитать pageCount из заголовка
  void saveHeader(); // сохранить PageCount в заголовок
};

#endif // DBMS_PAIN_PAGEMANAGER_H