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
  Page ReadPage(PageID_t pageID);

  // Записать страницу из памяти на диск
  void WritePage(PageID_t pageID, const Page &page);

  // Выделить новую страницу (расширить файл)
  PageID_t AllocatePage();

  // Пометить страницу как свободную
  void FreePage(PageID_t pageID);

  // Сколько страниц в файле
  int32_t PageCount() const;

private:
  std::string filePath_;
  std::fstream file_;
  int32_t pageCount_;
  int32_t freeListHead_;

  void InitFile();   // создать файл если не существует
  void LoadHeader(); // прочитать PageCount из заголовка
  void SaveHeader(); // сохранить PageCount в заголовок
};

#endif // DBMS_PAIN_PAGEMANAGER_H