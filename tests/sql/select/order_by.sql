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
-- error: Identifier 'nonexistent' not defined in table nor as an alias
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
-- | id | number |
-- |  1 |   2137 |
-- |  6 |   1234 |
-- |  3 |    420 |
-- |  0 |     69 |
-- |  4 |     69 |
-- |  5 |     69 |
-- |  2 |   null |
SELECT id, number FROM test ORDER BY 2 DESC, 1 ASC;

-- Disallow ordering by index with SELECT *
-- error: Index is not allowed when using SELECT *
SELECT * FROM test ORDER BY 2 DESC, 1 ASC;

-- Order by alias
-- output:
-- | id | number_alias |
-- |  1 |         2137 |
-- |  6 |         1234 |
-- |  3 |          420 |
-- |  0 |           69 |
-- |  4 |           69 |
-- |  5 |           69 |
-- |  2 |         null |
SELECT id, number AS number_alias FROM test ORDER BY number_alias DESC, 1 ASC;

-- Order by column name when alias is defined
-- output:
-- | number_alias |
-- |         2137 |
-- |         1234 |
-- |          420 |
-- |           69 |
-- |           69 |
-- |           69 |
-- |         null |
SELECT number AS number_alias FROM test ORDER BY number DESC;

-- Order by column name that is not included in SELECT clause
-- output:
-- | id |
-- |  1 |
-- |  6 |
-- |  3 |
-- |  0 |
-- |  4 |
-- |  5 |
-- |  2 |
SELECT id FROM test ORDER BY number DESC;
