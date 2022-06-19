IMPORT CSV 'aggregate.csv' INTO test;

-- error: Invalid column 'nonexistent' used in aggregate function
SELECT COUNT(nonexistent) FROM test;
