IMPORT CSV '../where/where.csv' INTO test;

-- error: Select expression must return a single row with a single value
SELECT (SELECT number FROM test) AS subquery;
