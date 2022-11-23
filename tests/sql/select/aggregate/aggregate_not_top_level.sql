IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | (AggregateFunction?(TODO) + 1) |
-- |                              8 |
SELECT COUNT(id) + 1 FROM test;
