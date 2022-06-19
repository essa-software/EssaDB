IMPORT CSV 'where.csv' INTO test;

-- output:
-- | id | number |
-- |  1 |   2137 |
-- |  5 |     69 |
SELECT id, number FROM test WHERE id IN(1, 5);
