IMPORT CSV 'aggregate.csv' INTO test;

-- error: Column 'nonexistent' does not exist in table 'test'
SELECT COUNT(nonexistent) FROM test;
