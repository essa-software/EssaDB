IMPORT CSV '../tests/sql/aggregate.csv' INTO test;

-- skip: segfaults
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group] ORDER BY COUNTED DESC;
