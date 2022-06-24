IMPORT CSV '../where/where.csv' INTO test;

-- error: Expression result must contain 1 row and 1 column in column value
SELECT (SELECT number FROM test) AS subquery;
