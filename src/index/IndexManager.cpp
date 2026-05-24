#include "IndexManager.h"

IndexManager::IndexManager(const std::string& filePath) : filePath_(filePath), intTree_(nullptr), strTree_(nullptr) {
    load();
}

IndexManager::~IndexManager() {
    save();
}

void IndexManager::load() {
    if (std::filesystem::exists(filePath_)) {
        std::ifstream file(filePath_, std::ios::binary);
        if (!file.is_open()) {
            throw IndexError("Failed to open file");
        }
        file.close();
    } else {
        std::filesystem::path path(filePath_);
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream file(filePath_, std::ios::binary);
        if (!file.is_open()) {
            throw IndexError("Failed to create index file: " + filePath_);
        }
        file.close()
    }
}

void IndexManager::save() {
    std::ifstream file(filePath_, std::ios::binary);
    file.close();

}

void IndexManager::insertKey(const Value& key, RecordID rID) {
    if (val::isNull(key)) {
        throw IndexError("Cannot insert NULL into INDEXED column");
    }
    if (key.has_value()) {
        if (std::holds_alternative<int>(key.value())) {
            intTree_ = std::unique_ptr<BspTree<int, RecordID>>();
        }
        auto val_int = std::get<int>(key.value());
        auto it = intTree_->find(val_int);
        if (intTree_->find(val_int) == intTree_->end()) {
            throw IndexError("duplicate");
        }
        intTree_->insert({val_int, rID});
    }
}

void IndexManager::removeKey(const Value& key) {
    if (std::holds_alternative<int>(key.value()) && intTree_) {
        auto iterator = intTree_->erase(std::get<int>(key.value()));
        if (iterator == intTree_->end()) {
            throw IndexError("no key to remove");
        }

    } else if (std::holds_alternative<std::string>(key.value()) && strTree_) {
        auto iterator = strTree_->erase(std::get<std::string>(key.value()));
        if (iterator == strTree_->end()) {
            throw IndexError("no key to remove");
        }
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
}

