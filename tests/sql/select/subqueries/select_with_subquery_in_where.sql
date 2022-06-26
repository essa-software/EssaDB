IMPORT CSV '../where/where.csv' INTO test;
IMPORT CSV '../where/where.csv' INTO newtest;

-- output:
-- | id | string |
-- |  0 |   test |
-- |  1 |   null |
-- |  2 |   null |
-- |  3 |   null |
-- |  4 |  test1 |
-- |  5 |  test2 |
-- |  6 |  testw |
SELECT t.id, t.string FROM test AS t WHERE (SELECT n.number FROM newtest AS n WHERE n.string = 'test') = 69;
