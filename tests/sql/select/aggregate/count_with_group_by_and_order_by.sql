IMPORT CSV 'aggregate.csv' INTO test;

-- skip won't work until proper alias support is implemented
-- Ordering by grouped by column
-- output:
-- | group | COUNTED | len |
-- |     C |       3 |   1 |
-- |     B |       2 |   1 |
-- |    AA |       2 |   2 |
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group] ORDER BY [group] DESC;
