-- РАЗДЕЛ 1: МЕТА-ОПЕРАЦИИ И КОНТЕКСТ
CREATE DATABASE main_db;
USE main_db;

-- РАЗДЕЛ 2: DDL - CREATE TABLE С МОДИФИКАТОРАМИ
-- Проверяем: NOT NULL, INDEXED, DEFAULT <value>
CREATE TABLE employees (
    id int NOT NULL INDEXED,
    name string NOT NULL,
    department string INDEXED,
    salary int DEFAULT 50000,
    status string DEFAULT "active",
    created_at int
);

CREATE TABLE users (
    id INT INDEXED,
    name STRING NOT NULL,
    age INT
);
-- РАЗДЕЛ 3: DML - INSERT
-- Вставка одной строки
INSERT INTO employees (id, name, department, salary, status) VALUE (1, "Alice", "IT", 70000, "active");
-- Вставка нескольких строк
INSERT INTO employees (id, name, department, salary) VALUE (2, "Bob", "HR", 60000), (3, "Charlie", "IT", 65000);
-- Вставка с пропуском колонок (сработают DEFAULT / NULL)
INSERT INTO employees (id, name, department) VALUE (4, "Dave", "Finance");

-- РАЗДЕЛ 4: DML - SELECT, АЛИАСТЫ И РЕГИСТР КЛЮЧЕВЫХ СЛОВ
-- Ключевые слова регистронезависимы, но смешение внутри слова запрещено
SELECT * FROM employees;
SELECT name AS full_name, salary AS pay FROM employees;
select id, name from employees;  -- валидно (нижний регистр)
SELECT ID, DEPARTMENT FROM employees; -- валидно (верхний регистр)

-- РАЗДЕЛ 5: WHERE, СРАВНЕНИЯ И ПРИОРИТЕТЫ (AND > OR)
-- Числовые и лексикографические сравнения
SELECT * FROM employees WHERE salary > 65000;
SELECT * FROM employees WHERE id = 1;
SELECT * FROM employees WHERE name >= "Alice" AND name < "Dave";
-- Приоритет AND над OR без скобок
SELECT * FROM employees WHERE department = "HR" OR department = "IT" AND salary > 60000;
-- Явное управление приоритетом скобками
SELECT * FROM employees WHERE (department = "HR" OR department = "IT") AND salary > 60000;

-- РАЗДЕЛ 6: BETWEEN И LIKE (REGEX)
-- BETWEEN: полуоткрытый интервал [start, end)
SELECT * FROM employees WHERE salary BETWEEN 50000 AND 70000;
-- LIKE принимает регулярные выражения (не SQL-паттерны)
SELECT * FROM employees WHERE name LIKE "^A.*";
SELECT * FROM employees WHERE department LIKE ".*R$";
SELECT * FROM employees WHERE name LIKE ".*li.*";

-- РАЗДЕЛ 7: АГРЕГАТНЫЕ ФУНКЦИИ
SELECT COUNT() FROM employees;
SELECT COUNT(id) FROM employees WHERE department = "IT";
SELECT SUM(salary) FROM employees WHERE department = "HR";
SELECT AVG(salary) FROM employees;
-- Несколько агрегатов в одном запросе
SELECT COUNT(), SUM(salary), AVG(salary) FROM employees WHERE status = "active";

-- РАЗДЕЛ 8: UPDATE И DELETE
UPDATE employees SET salary = 72000 WHERE id = 1;
UPDATE employees SET status = "inactive", department = "Archive" WHERE id = 3;
DELETE FROM employees WHERE id > 4;

-- РАЗДЕЛ 9: МНОГОСТРОЧНОСТЬ И ФОРМАТИРОВАНИЕ
SELECT
    id,
    name AS emp_name,
    salary AS monthly_pay
FROM employees
WHERE department = "IT"
AND salary BETWEEN 50000 AND 80000;

-- Игнорирование лишних пробелов/табуляций
SELECT  *  FROM   employees   WHERE   id = 1   ;

-- НЕГАТИВНЫЕ ТЕСТЫ (ДОЛЖНЫ ВЫЗЫВАТЬ ОШИБКУ ПАРСИНГА)

-- ОШИБКА: VALUES вместо VALUE (нарушение ТЗ)
INSERT INTO employees (id) VALUES (100);

-- ОШИБКА: Смешение регистров внутри ключевого слова (SeLeCt)
SeLeCt * FROM employees;

-- ОШИБКА: Отсутствует FROM
SELECT id, name;

-- ОШИБКА: Неподдерживаемый модификатор
CREATE TABLE t (id int UNIQUE);

-- ОШИБКА: GROUP BY не поддерживается ТЗ (только глобальные агрегаты)
SELECT department, COUNT() FROM employees GROUP BY department;

-- ОШИБКА: Пропущено условие в UPDATE
UPDATE employees SET salary WHERE id = 1;

-- КОНЕЦ ТЕСТОВ
