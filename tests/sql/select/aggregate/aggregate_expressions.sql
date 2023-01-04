IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | SUM((id + 1)) |
-- |     32.000000 |
SELECT SUM(id + 1) FROM test;
