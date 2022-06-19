IMPORT CSV 'where.csv' INTO test;

-- output:
-- | id |
SELECT id FROM test WHERE id = 2 AND id = 3;

-- output:
-- | id | number |
-- |  1 |   2137 |
SELECT id, number FROM test WHERE id = 1 AND number = 2137;
