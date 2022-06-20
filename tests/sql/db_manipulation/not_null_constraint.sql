CREATE TABLE test (id INT NOT NULL, value INT DEFAULT 3000);
INSERT INTO test (id, value) VALUES (2, 1000);
INSERT INTO test (id) VALUES (5);
-- error: Value can't be null.
INSERT INTO test (value) VALUES (1002);

-- output:
-- | id | value |
-- |  2 |  1000 |
-- |  5 |  3000 |
SELECT * FROM test;
