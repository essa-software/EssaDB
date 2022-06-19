IMPORT CSV 'where.csv' INTO test;

-- output:
-- | id | number |
-- |  2 |   null |
-- |  3 |    420 |
SELECT id, number FROM test WHERE id = 2 OR id = 3;
