-- output:
-- | UnaryOperator(10) |
-- |               -10 |
SELECT -10;

-- output:
-- | ArithmeticOperator(10,UnaryOperator(10)) |
-- |                                       20 |
SELECT 10 - -10;

-- Unary minus greater priority than arithmetic op
-- output:
-- | ArithmeticOperator(UnaryOperator(5),5) |
-- |                                      0 |
SELECT -5 + 5;
