CREATE TABLE test (id INT AUTO_INCREMENT, value INT);
INSERT INTO test (value) VALUES (1000);
INSERT INTO test (id, value) VALUES (5, 1001);
INSERT INTO test (value) VALUES (1002);

-- output:
-- | id | value |
-- |  1 |  1000 |
-- |  5 |  1001 |
-- |  2 |  1002 |
SELECT * FROM test;
