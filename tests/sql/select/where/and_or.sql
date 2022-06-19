IMPORT CSV 'where.csv' INTO test;

-- | id |
-- |  2 |
-- |  3 |
SELECT id FROM test WHERE id = 2 OR id = 3 AND number = 420;
