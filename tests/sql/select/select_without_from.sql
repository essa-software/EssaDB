-- output:
-- | 1 |
-- | 1 |
SELECT 1

-- output:
-- | ArithmeticOperator(1,4) |
-- |                       5 |
SELECT 1 + 4;

-- output:
-- | test |
-- |    1 |
SELECT 1 AS test;

-- error: You need a table to do SELECT *
SELECT *;

-- error: You need a table to resolve identifiers
SELECT test;

-- skip TODO: Support arbitrary expressions in aggregate functions, where they will behave like normal functions
SELECT COUNT(5);
