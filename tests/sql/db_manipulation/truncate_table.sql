IMPORT CSV 'data.csv' INTO test;

TRUNCATE TABLE test;

-- output:
-- Empty result set
SELECT * FROM test;
