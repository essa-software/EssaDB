IMPORT CSV 'distinct.csv' INTO test;

-- output:
-- | number | string |
-- |      1 |   test |
-- |      2 |    abc |
-- |      3 |    def |
-- |      4 |  siema |
-- |      5 |    tej |
-- |      1 |   2137 |
SELECT DISTINCT * FROM test;
