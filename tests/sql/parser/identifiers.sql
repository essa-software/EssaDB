-- output:
-- | test |
-- |    1 |
SELECT 1 AS test;

-- output:
-- | test1 |
-- |     1 |
SELECT 1 AS test1;

-- output:
-- | test_ |
-- |     1 |
SELECT 1 AS test_;

-- output:
-- | test_1 |
-- |      1 |
SELECT 1 AS test_1;

-- output:
-- | _test1 |
-- |      1 |
SELECT 1 AS _test1;

-- error: Expected identifier in alias, got '1'
SELECT 1 AS 1test;
