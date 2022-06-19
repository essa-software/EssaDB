IMPORT CSV 'where.csv' INTO test;

-- output:
-- | id | number |
-- |  2 |   null |
-- |  3 |    420 |
-- |  4 |     69 |
SELECT id, number FROM test WHERE id BETWEEN 2 AND 4;
