-- output:
-- | 1 |
-- | 1 |
SELECT 1

-- output:
-- | (1 + 4) |
-- |       5 |
SELECT 1 + 4;

-- output:
-- | test |
-- |    1 |
SELECT 1 AS test;

-- error: You need a table to do SELECT *
SELECT *;

-- error: Identifiers cannot be resolved without table
SELECT test;

-- output:
-- | AggregateFunction?(TODO) |
-- |                        1 |
SELECT COUNT(5);
