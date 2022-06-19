IMPORT CSV 'data.csv' INTO test;

TRUNCATE TABLE test;

-- output:
-- | id | number | string | integer |
SELECT * FROM test;
