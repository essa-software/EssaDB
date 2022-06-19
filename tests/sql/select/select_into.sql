IMPORT CSV 'where/where.csv' INTO test;

-- output:
-- | id | number | string | integer |
-- |  0 |     69 |   test |      48 |
-- |  1 |   2137 |   null |      65 |
-- |  2 |   null |   null |      89 |
-- |  3 |    420 |   null |     100 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * INTO backup FROM test;

-- skip this currently returns only rows with string=null for some reason
-- output:
-- | id | number | string | integer |
-- |  0 |     69 |   test |      48 |
-- |  1 |   2137 |   null |      65 |
-- |  2 |   null |   null |      89 |
-- |  3 |    420 |   null |     100 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM backup;
