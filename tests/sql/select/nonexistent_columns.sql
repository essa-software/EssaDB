CREATE TABLE test (id INT);

-- error: Column 'nonexistent' does not exist in table 'test'
SELECT nonexistent FROM test;

INSERT INTO test (id) VALUES (0);

-- error: Column 'nonexistent' does not exist in table 'test'
SELECT nonexistent FROM test;

-- in expression
-- error: Column 'nonexistent' does not exist in table 'test'
SELECT nonexistent + 54 FROM test;
