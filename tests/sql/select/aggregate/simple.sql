IMPORT CSV 'aggregate.csv' INTO test;

SELECT * FROM test;

-- TODO: Automatically convert SelectResult to value if value is given here

-- output:
-- | AggregateFunction?(TODO) |
-- |                        7 |
SELECT COUNT(id) FROM test;

-- output:
-- | AggregateFunction?(TODO) |
-- |                25.000000 |
SELECT SUM(id) FROM test;

-- output:
-- | AggregateFunction?(TODO) |
-- |                 0.000000 |
SELECT MIN(id) FROM test;

-- output:
-- | AggregateFunction?(TODO) |
-- |                 7.000000 |
SELECT MAX(id) FROM test;

-- output:
-- | AggregateFunction?(TODO) |
-- |                 3.125000 |
SELECT AVG(id) FROM test;
