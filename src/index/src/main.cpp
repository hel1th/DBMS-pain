#include <iostream>
#include <cassert>
#include <filesystem>
#include "../include/IndexManager.h"

// Вспомогательная функция для генерации фейковых RecordID
RecordID makeRecordID(int page, int slot) {
    RecordID rid;
    // Настрой эту часть под структуру твоего RecordID
    // Например: rid.pageID = page; rid.slotID = slot;
    return rid;
}

void test_1_integer_insert_and_find() {
    std::cout << "Test 1: Integer Insert and Find... ";
    std::string path = "test_int.idx";
    std::filesystem::remove(path); // очищаем перед тестом

    IndexManager idx(path);
    // Вставляем значение
    idx.insertKey(Value(42), makeRecordID(1, 10));
    
    // Ищем значение
    RecordID result = idx.findKey(Value(42));
    
    std::cout << "PASSED\n";
}

void test_2_persistence_save_and_load() {
    std::cout << "Test 2: Persistence (Save and Load)... ";
    std::string path = "test_persist.idx";
    std::filesystem::remove(path);

    {
        // Создаем менеджер, вставляем данные. 
        // При выходе из блока { } сработает деструктор и вызовет save()
        IndexManager idx(path);
        idx.insertKey(Value(100), makeRecordID(2, 20));
        idx.insertKey(Value(200), makeRecordID(2, 21));
    }

    {
        // Новый менеджер должен прочитать файл test_persist.idx в конструкторе
        IndexManager idx_loaded(path);
        RecordID result = idx_loaded.findKey(Value(200));
        // Если дошли сюда и не словили IndexError - индекс успешно загружен
    }
    
    std::cout << "PASSED\n";
}

void test_3_string_insert_and_find() {
    std::cout << "Test 3: String Insert and Find... ";
    std::string path = "test_str.idx";
    std::filesystem::remove(path);

    IndexManager idx(path);
    idx.insertKey(Value(std::string("hello")), makeRecordID(3, 30));
    idx.insertKey(Value(std::string("world")), makeRecordID(3, 31));

    RecordID result = idx.findKey(Value(std::string("hello")));
    
    std::cout << "PASSED\n";
}

void test_4_remove_key() {
    std::cout << "Test 4: Remove Key... ";
    std::string path = "test_remove.idx";
    std::filesystem::remove(path);

    IndexManager idx(path);
    idx.insertKey(Value(999), makeRecordID(4, 40));
    
    // Удаляем
    idx.removeKey(Value(999));
    
    // Пытаемся найти удаленный ключ. Должно выбросить исключение.
    bool threw_exception = false;
    try {
        idx.findKey(Value(999));
    } catch (const IndexError& e) {
        threw_exception = true;
    }
    
    assert(threw_exception && "Key should have been removed!");
    std::cout << "PASSED\n";
}

void test_5_duplicate_insert_throws() {
    std::cout << "Test 5: Duplicate Insert Throws... ";
    std::string path = "test_dup.idx";
    std::filesystem::remove(path);

    IndexManager idx(path);
    idx.insertKey(Value(777), makeRecordID(5, 50));
    
    bool threw_exception = false;
    try {
        // Пытаемся вставить тот же ключ
        idx.insertKey(Value(777), makeRecordID(5, 51));
    } catch (const IndexError& e) {
        threw_exception = true;
    }
    
    assert(threw_exception && "Duplicate insert should throw an error!");
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "--- Starting IndexManager Tests ---\n";
    try {
        test_1_integer_insert_and_find();
        test_2_persistence_save_and_load();
        test_3_string_insert_and_find();
        test_4_remove_key();
        test_5_duplicate_insert_throws();
        std::cout << "--- All tests completed successfully! ---\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTEST FAILED with exception: " << e.what() << "\n";
    }
    return 0;
}