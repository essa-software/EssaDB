-- addition
-- output:
-- | ArithmeticOperator(69,5) |
-- |                       74 |
SELECT 69 + 5;

-- output:
-- | ArithmeticOperator(69,null) |
-- |                        null |
SELECT 69 + NULL;

-- output:
-- | ArithmeticOperator(null,5) |
-- |                       null |
SELECT NULL + 5;

-- subtraction
-- output:
-- | ArithmeticOperator(69,5) |
-- |                       64 |
SELECT 69 - 5;

-- output:
-- | ArithmeticOperator(69,null) |
-- |                        null |
SELECT 69 - NULL;

-- output:
-- | ArithmeticOperator(null,5) |
-- |                       null |
SELECT NULL - 5;

-- multiplication
-- output:
-- | ArithmeticOperator(69,5) |
-- |                      345 |
SELECT 69 * 5;

-- output:
-- | ArithmeticOperator(69,null) |
-- |                        null |
SELECT 69 * NULL;

-- output:
-- | ArithmeticOperator(null,5) |
-- |                       null |
SELECT NULL * 5;

-- output:
-- | ArithmeticOperator(ArithmeticOperator(33,36),5) |
-- |                                             345 |
SELECT (33 + 36) * 5;

-- output:
-- | ArithmeticOperator(5,ArithmeticOperator(33,36)) |
-- |                                             345 |
SELECT 5 * (33 + 36);

-- division
-- output:
-- | ArithmeticOperator(69,5) |
-- |                       13 |
SELECT 69 / 5;

-- output:
-- | ArithmeticOperator(69,null) |
-- |                        null |
SELECT 69 / NULL;

-- output:
-- | ArithmeticOperator(null,5) |
-- |                       null |
SELECT NULL / 5;

-- division
-- error: Cannot divide by 0
SELECT 69 / 0;
