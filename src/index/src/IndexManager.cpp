#include "IndexManager.h"
#include <cstddef>

IndexManager::IndexManager(const std::string& filePath) :
    filePath_(filePath), intTree_(nullptr), strTree_(nullptr) {
    load();
}

IndexManager::~IndexManager() { save(); }

void IndexManager::load() {
    if (!std::filesystem::exists(filePath_)) {
        return;
    }
    std::ifstream file(filePath_, std::ios::binary);
    if (!file.is_open()) {
        throw IndexError("Failed to open file");
    }
    char type;
    if (!file.read(&type, sizeof(type))) {
        return;
    }
    if (type == 'I') {
        intTree_ = std::make_unique<BspTree<int, RecordID>>();
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (size_t i = 0; i < count; ++i) {
            int key;
            RecordID recordID;
            file.read(reinterpret_cast<char*>(&key), sizeof(key));
            file.read(reinterpret_cast<char*>(&recordID), sizeof(recordID));
            intTree_->insert({key, recordID});
        }
    } else if (type == 'S') {
        strTree_ = std::make_unique<BspTree<std::string, RecordID>>();
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (size_t i = 0; i < count; ++i) {
            size_t str_len;
            file.read(reinterpret_cast<char*>(&str_len), sizeof(str_len));
            std::string key(str_len, '\0');
            file.read(&key[0], str_len);
            RecordID recordID;
            file.read(reinterpret_cast<char*>(&recordID), sizeof(recordID));
            strTree_->insert({key, recordID});
        }
    }
    file.close();
}

void IndexManager::save() {
    std::ofstream file(filePath_, std::ios::binary);
    if (!file.is_open()) {
        throw IndexError("Failed to open file");
    }
    if (intTree_) {
        char type = 'I';
        file.write(&type, sizeof(type));

        size_t count = intTree_->size();
        file.write(reinterpret_cast<char*>(&count), sizeof(count));

        for (const auto& pair: *intTree_) {
            file.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
            file.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
        }

    } else if (strTree_) {
        char type = 'S';
        file.write(&type, sizeof(type));

        size_t count = strTree_->size();
        file.write(reinterpret_cast<char*>(&count), sizeof(count));

        for (const auto& pair: *strTree_) {
            size_t str_len = pair.first.size();
            file.write(reinterpret_cast<char*>(&str_len), sizeof(str_len));
            file.write(pair.first.data(), str_len);

            file.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
        }
    } else {
        char type = 'N';
        file.write(&type, sizeof(type));
    }
    file.close();
}

void IndexManager::insertKey(const Value& key, RecordID rID) {
    if (val::isNull(key)) {
        throw IndexError("Cannot insert NULL into INDEXED column");
    }
    if (key.has_value()) {
        if (std::holds_alternative<int>(key.value())) {
            if (!intTree_) {
                intTree_ = std::make_unique<BspTree<int, RecordID>>();
            }
            auto valInt = std::get<int>(key.value());
            if (intTree_->find(valInt) != intTree_->end()) {
                throw IndexError("duplicate");
            }
            intTree_->insert({valInt, rID});
        } else if (std::holds_alternative<std::string>(key.value())) {
            if (!strTree_) {
                strTree_ = std::make_unique<BspTree<std::string, RecordID>>();
            }
            auto valStr = std::get<std::string>(key.value());
            if (strTree_->find(valStr) != strTree_->end()) {
                throw IndexError("duplicate");
            }
            strTree_->insert({valStr, rID});
        }
    }
}

void IndexManager::removeKey(const Value& key) {
    if (std::holds_alternative<int>(key.value()) && intTree_) {
        int valInt = std::get<int>(key.value());
        if (intTree_->find(valInt) == intTree_->end()) {
            throw IndexError("no key to remove");
        }
        intTree_->erase(std::get<int>(key.value()));
    } else if (std::holds_alternative<std::string>(key.value()) && strTree_) {
        const std::string& strInt = std::get<std::string>(key.value());
        if (strTree_->find(strInt) == strTree_->end()) {
            throw IndexError("no key to remove");
        }
        intTree_->erase(std::get<int>(key.value()));
    }
}

RecordID IndexManager::findKey(const Value& key) {
    if (std::holds_alternative<int>(key.value()) && intTree_) {
        int keyInt = std::get<int>(key.value());
        auto iterator = intTree_->find(keyInt);
        if (iterator != intTree_->end()) {
            return iterator->second;
        } else {
            throw IndexError("Key not found");
        }
    } else if (std::holds_alternative<std::string>(key.value()) && strTree_) {
        const std::string& keyStr = std::get<std::string>(key.value());
        auto iterator = strTree_->find(keyStr);
        if (iterator != strTree_->end()) {
            return iterator->second;
        } else {
            throw IndexError("key not found");
        }
    }
    throw IndexError("Index key is empty or type is undefined");
}
