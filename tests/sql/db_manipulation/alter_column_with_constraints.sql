CREATE TABLE test (id INT, value INT);
ALTER TABLE test ALTER COLUMN id INT AUTO_INCREMENT PRIMARY KEY;

INSERT INTO test (value) VALUES (1000);
INSERT INTO test (value) VALUES (1001);
INSERT INTO test (value) VALUES (1002);

-- output:
-- | id | value |
-- |  1 |  1000 |
-- |  2 |  1001 |
-- |  3 |  1002 |
SELECT * FROM test;
