IMPORT CSV 'lower_upper.csv' INTO test;

-- output:
-- |    upper |
-- |     TEST |
-- |     TEST |
-- |     TEST |
-- |     TEST |
-- |     TEST |
-- | TEST YAY |
-- |  NOT YAY |
SELECT UPPER(string) AS [upper] FROM test;

-- output:
-- |    lower |
-- |     test |
-- |     test |
-- |     test |
-- |     test |
-- |     test |
-- | test yay |
-- |  not yay |
SELECT LOWER(string) AS [lower] FROM test;
