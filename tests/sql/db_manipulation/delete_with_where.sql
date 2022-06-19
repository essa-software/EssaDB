IMPORT CSV 'data.csv' INTO test;

DELETE FROM test WHERE string LIKE 'te*'

-- output:
-- | id | number | string | integer |
-- |  1 |   2137 |   null |      65 |
-- |  2 |   null |   null |      89 |
-- |  3 |    420 |   null |     100 |
SELECT * FROM test;
