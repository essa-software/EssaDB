CREATE TABLE IF NOT EXISTS test;

DROP TABLE IF EXISTS test;

-- error: Nonexistent table: test
SELECT * FROM test;
