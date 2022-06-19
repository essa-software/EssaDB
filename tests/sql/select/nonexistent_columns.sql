CREATE TABLE test (id INT);

-- error: No such column: nonexistent
SELECT nonexistent FROM test;

INSERT INTO test (id) VALUES (0);

-- error: No such column: nonexistent
SELECT nonexistent FROM test;

-- in expression
-- error: No such column: nonexistent
SELECT nonexistent + 54 FROM test;
