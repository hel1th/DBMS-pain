CREATE DATABASE test_db;
USE test_db;

CREATE TABLE employees (
    id INT INDEXED,
    name STRING NOT NULL,
    role STRING DEFAULT "developer",
    salary INT DEFAULT 50000);

INSERT INTO employees (id, name, role, salary) VALUE (1, "hel1th", "teamlead", 120000);
INSERT INTO employees (id, name, salary) VALUE (2, "alex_popa", 60000);
INSERT INTO employees (id, name, role) VALUE (3, "johny_suka", "manager");
INSERT INTO employees (id, name) VALUE (4, "vlad_normal");

SELECT * FROM employees;

SELECT id AS emp_id, name AS emp_name FROM employees WHERE salary > 55000;

SELECT * FROM employees WHERE name LIKE ".*popa.*";

SELECT * FROM employees WHERE salary BETWEEN 45000 AND 70000;

UPDATE employees SET salary = 65000 WHERE id = 4;

SELECT * FROM employees WHERE id = 4;

DELETE FROM employees WHERE id = 3;

SELECT * FROM employees;