# Папки и цели
BUILD_DIR = build
DATA_DIR = data
TARGET = $(BUILD_DIR)/DBMS_pain

.PHONY: all clean clean-data run

# Цель по умолчанию: конфигурирует (если нужно) и собирает проект в 14 потоков
all:
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR) -j6
	@echo "=== Сборка успешно завершена ==="

# Твоя прошлая команда полной очистки (удаляет весь кэш сборки)
clean:
	rm -rf $(BUILD_DIR)
	@echo "=== Папка build полностью удалена ==="

# Очистка только файлов базы данных перед тестами
clean-data:
	@mkdir -p $(DATA_DIR)
	rm -rf $(DATA_DIR)
	@echo "=== Данные БД в папке '$(DATA_DIR)' успешно очищены ==="

# Автоматизация: инкрементальная сборка, очистка данных и запуск теста
run: all clean-data
	@echo "=== Запуск тестового сценария ==="
	./$(TARGET) src/test_workflow.sql