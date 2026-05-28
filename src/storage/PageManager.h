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
  Page readPage(PageID_t pageID);
  void writePage(PageID_t pageID, const Page &page);
  PageID_t allocatePage();
  void freePage(PageID_t pageID);
  int32_t pageCount() const;

private:
  std::string filePath_;
  std::fstream file_;
  int32_t pageCount_;
  int32_t freeListHead_;

  void initFile();
  void loadHeader();
  void saveHeader();
};

#endif // DBMS_PAIN_PAGEMANAGER_H