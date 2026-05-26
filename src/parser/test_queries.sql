-- Валидные запросы
SELECT * FROM users;
SELECT name, age FROM users WHERE age > 18;
SELECT name FROM users WHERE age > 18 AND name != "Bob";
INSERT INTO users (name, age) VALUES ("Alice", 20);
UPDATE users SET age = 21 WHERE name == "Alice";
DELETE FROM users WHERE age < 18;
CREATE TABLE students (id int, name string NOT NULL, age int INDEXED);
DROP TABLE students;
CREATE DATABASE testdb;
DROP DATABASE testdb;
USE testdb;

-- Запросы с BETWEEN и LIKE
SELECT * FROM products WHERE price BETWEEN 100 AND 500;
SELECT name FROM users WHERE name LIKE "A%";

-- Запросы со скобками и AND/OR
SELECT * FROM orders WHERE (status == "paid" AND amount > 1000) OR priority == "high";

-- Невалидные запросы (должны вызвать ошибку)
SELECT FROM users;
INSERT INTO users VALUES (1, 2);
UPDATE users SET age WHERE name == "Bob";
DELETE FROM;
CREATE TABLE t (id int NOT NULL INDEXED UNSUPPORTED);