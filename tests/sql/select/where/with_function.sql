IMPORT CSV 'where.csv' INTO test;

-- output:
-- | id | string | LEN(TODO) |
-- |  0 |   test |         4 |
SELECT id, string, LEN(string) FROM test WHERE LEN(string) = 4;
