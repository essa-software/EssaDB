CREATE TABLE test (id INT, value VARCHAR);

-- error: Table 'test' already exists
CREATE TABLE test (id INT, value VARCHAR);

-- Different structure.
-- error: Table 'test' already exists
CREATE TABLE test (id INT);
