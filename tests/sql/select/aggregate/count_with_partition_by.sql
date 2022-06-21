IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | group | COUNTED |  len |
-- |    AA |       2 |    2 |
-- |  null |       2 | null |
-- |     6 |       2 |    1 |
-- |     B |       2 |    1 |
-- |     7 |       2 |    1 |
-- |     C |       3 |    1 |
-- |     4 |       3 |    1 |
-- |     2 |       3 |    1 |
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test PARTITION BY [group];
