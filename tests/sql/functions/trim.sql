IMPORT CSV 'functions.csv' INTO test;

-- ltrim
-- | id | LENGTH |       TRIMMED |
-- |  0 |     15 |  123   456    |
-- |  1 |     12 |   12 34 56    |
-- |  2 |     15 | 1 2  3   4    |
-- |  3 |     10 |    123 456    |
-- |  4 |     10 |       123 456 |
-- |  5 |      7 |       123 456 |
SELECT id, LEN([to_trim]) AS [LENGTH], LTRIM([to_trim]) AS [TRIMMED] FROM test;

-- rtrim
-- | id | LENGTH |      TRIMMED |
-- |  0 |     15 |    123   456 |
-- |  1 |     12 |     12 34 56 |
-- |  2 |     15 |   1 2  3   4 |
-- |  3 |     10 |      123 456 |
-- |  4 |     10 |      123 456 |
-- |  5 |      7 |      123 456 |
SELECT id, LEN([to_trim]) AS [LENGTH], RTRIM([to_trim]) AS [TRIMMED] FROM test;

-- trim
-- | id | LENGTH |    TRIMMED |
-- |  0 |     15 |  123   456 |
-- |  1 |     12 |   12 34 56 |
-- |  2 |     15 | 1 2  3   4 |
-- |  3 |     10 |    123 456 |
-- |  4 |     10 |    123 456 |
-- |  5 |      7 |    123 456 |
SELECT id, LEN([to_trim]) AS [LENGTH], TRIM([to_trim]) AS [TRIMMED] FROM test;
