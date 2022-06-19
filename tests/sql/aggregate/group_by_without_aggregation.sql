IMPORT CSV 'aggregate.csv' INTO test;

-- output:
-- |   id | group |
-- | null |    AA |
-- |    1 |    AA |
-- |    2 |     C |
-- |    3 |     B |
-- |    4 |     C |
-- |    6 |    AA |
-- |    7 |     B |
SELECT id, [group] FROM test GROUP BY id, [group];
