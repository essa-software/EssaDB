IMPORT CSV 'aggregate.csv' INTO test;

-- display
SELECT * FROM test;

-- skip: segfaults
SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group] ORDER BY COUNTED DESC;
