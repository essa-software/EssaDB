IMPORT CSV 'data.csv' INTO test;

ALTER TABLE test ADD new VARCHAR;

-- output:
-- | id | number | string | integer |  new |
-- |  0 |     69 |   test |      48 | null |
-- |  1 |   2137 |   null |      65 | null |
-- |  2 |   null |   null |      89 | null |
-- |  3 |    420 |   null |     100 | null |
-- |  4 |     69 |  test1 |     122 | null |
-- |  5 |     69 |  test2 |      58 | null |
-- |  6 |   1234 |  testw |     165 | null |
SELECT * FROM test;
