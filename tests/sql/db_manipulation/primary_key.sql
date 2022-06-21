CREATE TABLE test (id INT PRIMARY KEY, value INT DEFAULT 3000);
INSERT INTO test (id, value) VALUES (2, 1000);
-- error: Not valid UNIQUE value.
INSERT INTO test (id) VALUES (2);
-- error: Value can't be null.
INSERT INTO test (value) VALUES (1002);
INSERT INTO test (id, value) VALUES (5, 1003);

-- output:
-- | id | value |
-- |  2 |  1000 |
-- |  5 |  1003 |
SELECT * FROM test;
