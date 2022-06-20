CREATE TABLE test (id INT UNIQUE, value INT DEFAULT 3000);
INSERT INTO test (id, value) VALUES (2, 1000);
INSERT INTO test (id) VALUES (5);
-- error: Not valid UNIQUE value.
INSERT INTO test (id, value) VALUES (2, 1002);

-- output:
-- | id | value |
-- |  2 |  1000 |
-- |  5 |  3000 |
SELECT * FROM test;
