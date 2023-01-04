IMPORT CSV 'aggregate.csv' INTO test;

SELECT * FROM test;

-- TODO: Automatically convert SelectResult to value if value is given here

-- output:
-- | COUNT(id) |
-- |         7 |
SELECT COUNT(id) FROM test;

-- output:
-- |   SUM(id) |
-- | 25.000000 |
SELECT SUM(id) FROM test;

-- output:
-- |  MIN(id) |
-- | 0.000000 |
SELECT MIN(id) FROM test;

-- output:
-- |  MAX(id) |
-- | 7.000000 |
SELECT MAX(id) FROM test;

-- output:
-- |  AVG(id) |
-- | 3.125000 |
SELECT AVG(id) FROM test;
