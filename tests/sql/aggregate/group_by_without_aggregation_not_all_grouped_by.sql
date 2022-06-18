IMPORT CSV '../tests/sql/aggregate.csv' INTO test;

-- error: Column 'id' must be either aggregate or occur in GROUP BY clause
SELECT id, [group] FROM test GROUP BY [group];
