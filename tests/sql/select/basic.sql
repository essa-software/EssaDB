CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT, date TIME);
INSERT INTO test (id, number, string, integer, date) VALUES (0, 69, 'test', 48, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (1, 2137, 65, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (2, null, 89, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (3, 420, 100, #1990-02-12#);
INSERT INTO test (id, number, string, integer, date) VALUES (4, 69, 'test1', 122, #1990-02-12#);
INSERT INTO test (id, number, string, integer, date) VALUES (5, 69, 'test2', 58, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (3, 420, 100, #1990-02-12#);

-- Select all
-- output:
-- | id | number | string | integer |       date |
-- |  0 |     69 |   test |      48 | 1990-02-12 |
-- |  1 |   2137 |   null |      65 | 1990-02-12 |
-- |  2 |   null |   null |      89 | 1990-02-12 |
-- |  3 |    420 |   null |     100 | 1990-02-12 |
-- |  4 |     69 |  test1 |     122 | 1990-02-12 |
-- |  5 |     69 |  test2 |      58 | 1990-02-12 |
-- |  3 |    420 |   null |     100 | 1990-02-12 |
SELECT * FROM test;

-- Select specific
-- output:
-- | number | string |
-- |     69 |   test |
-- |   2137 |   null |
-- |   null |   null |
-- |    420 |   null |
-- |     69 |  test1 |
-- |     69 |  test2 |
-- |    420 |   null |
SELECT number, string FROM test;

-- Top X
-- output:
-- | id | number | string | integer |       date |
-- |  0 |     69 |   test |      48 | 1990-02-12 |
-- |  1 |   2137 |   null |      65 | 1990-02-12 |
SELECT TOP 2 * FROM test;

-- Top perc
-- output:
-- | id | number | string | integer |       date |
-- |  0 |     69 |   test |      48 | 1990-02-12 |
-- |  1 |   2137 |   null |      65 | 1990-02-12 |
-- |  2 |   null |   null |      89 | 1990-02-12 |
-- |  3 |    420 |   null |     100 | 1990-02-12 |
-- |  4 |     69 |  test1 |     122 | 1990-02-12 |
SELECT TOP 75 PERC * FROM test;

-- Top on empty set
CREATE TABLE empty_test (id INT);

-- output:
-- Empty result set
SELECT TOP 5 * FROM empty_test;

-- output:
-- Empty result set
SELECT TOP 50 PERC * FROM empty_test;

-- error: Expected integer for TOP value, got 'this is not an int'
SELECT TOP 'this is not an int' PERC * FROM test;

-- Top where selected count > result set size
INSERT INTO empty_test VALUES(0);
INSERT INTO empty_test VALUES(1);
INSERT INTO empty_test VALUES(2);
INSERT INTO empty_test VALUES(3);

-- output:
-- | id |
-- |  0 |
-- |  1 |
-- |  2 |
-- |  3 |
SELECT TOP 10 * FROM empty_test;

-- output:
-- | id |
-- |  0 |
-- |  1 |
-- |  2 |
-- |  3 |
SELECT TOP 150 PERC * FROM empty_test;

-- Aliases
-- output:
-- | ID |  NUM |   STR |
-- |  0 |   69 |  test |
-- |  1 | 2137 |  null |
-- |  2 | null |  null |
-- |  3 |  420 |  null |
-- |  4 |   69 | test1 |
-- |  5 |   69 | test2 |
-- |  3 |  420 |  null |
SELECT id AS ID, number AS NUM, string AS STR FROM test;

-- Identifier trimming
-- output:
-- | id | some number | some  string |
-- |  0 |          69 |         test |
-- |  1 |        2137 |         null |
-- |  2 |        null |         null |
-- |  3 |         420 |         null |
-- |  4 |          69 |        test1 |
-- |  5 |          69 |        test2 |
-- |  3 |         420 |         null |
SELECT id AS [id], number AS [some number], string AS [ some  string ] FROM test;

-- Case
-- output:
-- | ID | CASE RESULT |
-- |  0 | less than 2 |
-- |  1 | less than 2 |
-- |  2 | less than 4 |
-- |  3 | less than 4 |
-- |  4 | other cases |
-- |  5 | other cases |
-- |  3 | less than 4 |
SELECT id AS [ID], CASE WHEN id < 2 THEN 'less than 2' WHEN id < 4 THEN 'less than 4' ELSE 'other cases' END AS [CASE RESULT] FROM test;
