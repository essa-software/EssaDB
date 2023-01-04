IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- | (COUNT(id) + 1) |
-- |               8 |
SELECT COUNT(id) + 1 FROM test;
