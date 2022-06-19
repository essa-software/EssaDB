IMPORT CSV 'where.csv' INTO test;

-- skip fails with 'test' is not a valid int
SELECT * FROM test WHERE string IS NULL;

-- output:
-- | id | number | string | integer |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string IS NOT NULL;
