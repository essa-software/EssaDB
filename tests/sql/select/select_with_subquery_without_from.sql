IMPORT CSV 'where/where.csv' INTO test;

-- output:
-- | subquery |
-- |        2 |
SELECT (SELECT TOP 1 id FROM test) AS subquery;
