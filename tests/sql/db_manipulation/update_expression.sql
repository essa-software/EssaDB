IMPORT CSV 'data.csv' INTO test;

UPDATE test SET id = id + 5

-- output:
-- | id | number | string | integer |
-- |  5 |     69 |   test |      48 |
-- |  6 |   2137 |   null |      65 |
-- |  7 |   null |   null |      89 |
-- |  8 |    420 |   null |     100 |
-- |  9 |     69 |  test1 |     122 |
-- | 10 |     69 |  test2 |      58 |
-- | 11 |   1234 |  testw |     165 |
SELECT * FROM test;
