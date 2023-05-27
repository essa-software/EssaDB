-- output:
-- | 2022-11-11 00:00:00 |
-- | 2022-11-11 00:00:00 |
SELECT #2022-11-11#;

-- error: Invalid ISO8601 date
SELECT #2022-11-1#;

-- error: Invalid ISO8601 date
SELECT #2022-1-11#;

-- error: Invalid ISO8601 date
SELECT #202-11-11#;

-- error: Invalid ISO8601 date
SELECT #2022-11-11foo#;
