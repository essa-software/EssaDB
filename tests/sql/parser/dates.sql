-- output:
-- | 2022-11-11 21:37:00 |
-- | 2022-11-11 21:37:00 |
SELECT #2022-11-11 21:37:00#;

-- output:
-- |   Formatted |
-- | 15 Jun 2017 |
SELECT DATE_FORMAT(#2017-06-15#, '%e %b %Y') AS [Formatted];

-- skip: shit varies
SELECT GETDATE();

-- skip: shit varies
SELECT GETUTCDATE();

-- skip: shit varies
SELECT SYSGETTIME();

-- error: Invalid ISO8601 date
SELECT #2022-11-1#;

-- error: Invalid ISO8601 date
SELECT #2022-1-11#;

-- error: Invalid ISO8601 date
SELECT #202-11-11#;

-- error: Invalid ISO8601 date
SELECT #2022-11-11foo#;
