#ifndef DBMS_PAIN_INDEXMANAGER_H
#define DBMS_PAIN_INDEXMANAGER_H
#include <string>
#include "BStarPlusTree.h"
#include "../storage/RecordManager.h"
#include "../utils/Error.h"
#include <fstream>
#include <filesystem>
#include <functional>

class IndexManager {
public:
    // Открывает файл индекса. Если файл существует — загружает дерево.
    // Если нет — создаёт пустое дерево и файл.
    explicit IndexManager(const std::string& filePath);
    ~IndexManager();
    
    // запрещаем копирования индекс манагера
    IndexManager(const IndexManager&) = delete;
    IndexManager& operator=(const IndexManager&) = delete;

    // Вставить ключ -> RecordID (вызывается при INSERT или UPDATE)
    void insertKey(const Value& key, RecordID rID);

    // Найти RecordID по ключу (вызывается при SELECT)
    // Бросает IndexError если ключ не найден
    RecordID findKey(const Value& key);

    // Удалить ключ (вызывается при DELETE)
    void removeKey(const Value& key);


private:
    std::string filePath_;

    // все функции определяют тип Value и делешируют в нужное дерево
    std::unique_ptr<BspTree<int, RecordID>> intTree_;
    std::unique_ptr<BspTree<std::string, RecordID>> strTree_;

    void load(); // читает файл -> заполняет дерево
    void save(); // сериализует дерево -> пишет в файл

};

#endif // DBMS_PAIN_INDEXMANAGER_H