IMPORT CSV 'data.csv' INTO test;

UPDATE test SET id = 5;

-- output:
-- | id | number | string | integer |
-- |  5 |     69 |   test |      48 |
-- |  5 |   2137 |   null |      65 |
-- |  5 |   null |   null |      89 |
-- |  5 |    420 |   null |     100 |
-- |  5 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  5 |   1234 |  testw |     165 |
SELECT * FROM test;
