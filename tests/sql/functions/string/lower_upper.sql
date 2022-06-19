IMPORT CSV 'data.csv' INTO test;

-- upper
-- skip FIXME: this doesn't actually work
SELECT id, UPPER(string) AS [UPPER STRING] FROM test;

-- lower_upper
-- skip FIXME: this doesn't actually work
SELECT id, LOWER(string) AS [STRING] FROM test;
