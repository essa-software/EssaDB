IMPORT CSV 'aggregate.csv' INTO test;

-- error: Column 'id' must be either aggregate or occur in GROUP BY clause
SELECT COUNT(id), id FROM test;
