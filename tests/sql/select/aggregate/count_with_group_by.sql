IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | group | COUNTED | len |
-- |    AA |       2 |   2 |
-- |     B |       2 |   1 |
-- |     C |       3 |   1 |
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group];
