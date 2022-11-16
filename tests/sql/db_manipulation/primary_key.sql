CREATE TABLE test (id INT PRIMARY KEY, value INT DEFAULT 3000);
INSERT INTO test (id, value) VALUES (2, 1000);
-- error: Primary key must be unique
INSERT INTO test (id) VALUES (2);
-- error: Primary key may not be null
INSERT INTO test (value) VALUES (1002);
INSERT INTO test (id, value) VALUES (5, 1003);

-- output:
-- | id | value |
-- |  2 |  1000 |
-- |  5 |  1003 |
SELECT * FROM test;
