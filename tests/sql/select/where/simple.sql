IMPORT CSV 'where.csv' INTO test;

-- output:
-- | id | number | string | integer |
-- |  2 |   null |   null |      89 |
SELECT * FROM test WHERE id = 2;
