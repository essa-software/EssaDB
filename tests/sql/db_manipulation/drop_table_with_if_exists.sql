CREATE TABLE IF NOT EXISTS test;

-- error: Table 'test' already exists
CREATE TABLE test;

-- output:
-- null
CREATE TABLE IF NOT EXISTS test;

-- output:
-- null
DROP TABLE IF EXISTS test;

-- error: Nonexistent table: test
SELECT * FROM test;
