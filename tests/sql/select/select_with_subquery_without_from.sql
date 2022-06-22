IMPORT CSV 'where/where.csv' INTO test;

-- output:
-- | subquery |
-- |       69 |
SELECT (SELECT TOP 1 number FROM test) AS subquery;
