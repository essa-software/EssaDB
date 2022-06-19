IMPORT CSV 'where/where.csv' INTO test;

-- Order by ascending
-- output:
-- | id | number | string | integer |
-- |  2 |   null |   null |      89 |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  3 |    420 |   null |     100 |
-- |  6 |   1234 |  testw |     165 |
-- |  1 |   2137 |   null |      65 |
SELECT * FROM test ORDER BY number;

-- Order by ascending (explicit)
-- output:
-- | id | number | string | integer |
-- |  2 |   null |   null |      89 |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  3 |    420 |   null |     100 |
-- |  6 |   1234 |  testw |     165 |
-- |  1 |   2137 |   null |      65 |
SELECT * FROM test ORDER BY number ASC;

-- Order by descending (explicit)
-- output:
-- | id | number | string | integer |
-- |  1 |   2137 |   null |      65 |
-- |  6 |   1234 |  testw |     165 |
-- |  3 |    420 |   null |     100 |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  2 |   null |   null |      89 |
SELECT * FROM test ORDER BY number DESC;

-- Order by nonexistent
-- error: No such column: nonexistent
SELECT * FROM test ORDER BY nonexistent;

-- Order by multiple
-- output:
-- | id | number | string | integer |
-- |  1 |   2137 |   null |      65 |
-- |  6 |   1234 |  testw |     165 |
-- |  3 |    420 |   null |     100 |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  2 |   null |   null |      89 |
SELECT * FROM test ORDER BY number DESC, id ASC;

-- Order by index
-- output:
-- | id | number | string | integer |
-- |  1 |   2137 |   null |      65 |
-- |  6 |   1234 |  testw |     165 |
-- |  3 |    420 |   null |     100 |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  2 |   null |   null |      89 |
SELECT * FROM test ORDER BY 2 DESC, 1 ASC;
