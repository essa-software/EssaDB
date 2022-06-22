CREATE TABLE test (id INT);
INSERT INTO test (id) VALUES (0);
INSERT INTO test (id) VALUES (1);
INSERT INTO test (id) VALUES (2);

-- output:
-- | id |
-- |  0 |
-- |  1 |
-- |  2 |
SELECT test.id FROM test;
