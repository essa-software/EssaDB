IMPORT CSV 'data.csv' INTO test;

CREATE TABLE new_test (id INT, number INT, string VARCHAR, integer INT);
INSERT INTO new_test (id, number, string, integer) SELECT * FROM test WHERE id < 2;

-- output:
-- | id | number | string | integer |
-- |  0 |     69 |   test |      48 |
-- |  1 |   2137 |   null |      65 |
SELECT * FROM new_test;
