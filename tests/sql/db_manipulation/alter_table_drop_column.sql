IMPORT CSV 'data.csv' INTO test;

ALTER TABLE test DROP COLUMN string

-- output:
-- | id | number | integer |
-- |  0 |     69 |      48 |
-- |  1 |   2137 |      65 |
-- |  2 |   null |      89 |
-- |  3 |    420 |     100 |
-- |  4 |     69 |     122 |
-- |  5 |     69 |      58 |
-- |  6 |   1234 |     165 |
SELECT * FROM test;

ALTER TABLE test DROP COLUMN number, integer

-- output:
-- | id |
-- |  0 |
-- |  1 |
-- |  2 |
-- |  3 |
-- |  4 |
-- |  5 |
-- |  6 |
SELECT * FROM test;
