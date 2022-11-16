CREATE TABLE test (id INT AUTO_INCREMENT, value INT DEFAULT 3000 CHECK value > 1000);
-- error: Values doesn't match general check rule specified for this table
INSERT INTO test (value) VALUES (1000);
INSERT INTO test (id) VALUES (5);
INSERT INTO test (value) VALUES (1002);

-- output:
-- | id | value |
-- |  5 |  3000 |
-- |  1 |  1002 |
SELECT * FROM test;
