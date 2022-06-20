CREATE TABLE test (id INT AUTO_INCREMENT, value INT DEFAULT 3000);
INSERT INTO test (value) VALUES (1000);
INSERT INTO test (id) VALUES (5);
INSERT INTO test (value) VALUES (1002);

-- output:
-- | id | value |
-- |  1 |  1000 |
-- |  5 |  3000 |
-- |  2 |  1002 |
SELECT * FROM test;
