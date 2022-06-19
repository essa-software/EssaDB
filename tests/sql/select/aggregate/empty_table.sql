CREATE TABLE test (id INT);

-- output:
-- | AggregateFunction?(TODO) |
-- |                        0 |
SELECT COUNT(id) FROM test;
