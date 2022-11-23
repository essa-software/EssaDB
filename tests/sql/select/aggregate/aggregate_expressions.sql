IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | AggregateFunction?(TODO) |
-- |                32.000000 |
SELECT SUM(id + 1) FROM test;
