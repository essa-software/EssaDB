-- addition
-- output:
-- | (69 + 5) |
-- |       74 |
SELECT 69 + 5;

-- output:
-- | (69 + NULL) |
-- |        null |
SELECT 69 + NULL;

-- output:
-- | (NULL + 5) |
-- |       null |
SELECT NULL + 5;

-- subtraction
-- output:
-- | (69 - 5) |
-- |       64 |
SELECT 69 - 5;

-- output:
-- | (69 - NULL) |
-- |        null |
SELECT 69 - NULL;

-- output:
-- | (NULL - 5) |
-- |       null |
SELECT NULL - 5;

-- multiplication
-- output:
-- | (69 * 5) |
-- |      345 |
SELECT 69 * 5;

-- output:
-- | (69 * NULL) |
-- |        null |
SELECT 69 * NULL;

-- output:
-- | (NULL * 5) |
-- |       null |
SELECT NULL * 5;

-- output:
-- | ((33 + 36) * 5) |
-- |             345 |
SELECT (33 + 36) * 5;

-- output:
-- | (5 * (33 + 36)) |
-- |             345 |
SELECT 5 * (33 + 36);

-- division
-- output:
-- | (69 / 5) |
-- |       13 |
SELECT 69 / 5;

-- output:
-- | (69 / NULL) |
-- |        null |
SELECT 69 / NULL;

-- output:
-- | (NULL / 5) |
-- |       null |
SELECT NULL / 5;

-- division
-- error: Cannot divide by 0
SELECT 69 / 0;

-- precedence
-- output:
-- | ((5 - 5) - 2) |
-- |            -2 |
SELECT 5 - 5 - 2;
