IMPORT CSV 'aggregate.csv' INTO test;

-- Ordering by grouped by column
-- output:
-- | group | COUNTED | len |
-- |     C |       3 |   1 |
-- |     B |       2 |   1 |
-- |    AA |       2 |   2 |
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group] ORDER BY [group] DESC;

-- Ordering by aggregated column
-- output:
-- | group | COUNTED | len |
-- |     C |       3 |   1 |
-- |    AA |       2 |   2 |
-- |     B |       2 |   1 |
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group] ORDER BY [COUNTED] DESC;
