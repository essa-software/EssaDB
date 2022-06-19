IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | group | COUNTED | len |
-- |    AA |       1 |   2 |
-- |     B |       1 |   1 |
-- |     C |       3 |   1 |
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test WHERE id < 6 GROUP BY [group];
