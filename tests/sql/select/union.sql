CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT, date TIME);
INSERT INTO test (id, number, string, integer, date) VALUES (0, 69, 'test', 48, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (1, 2137, 65, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (2, null, 89, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (3, 420, 100, #1990-02-12#);
INSERT INTO test (id, number, string, integer, date) VALUES (4, 69, 'test1', 122, #1990-02-12#);
INSERT INTO test (id, number, string, integer, date) VALUES (5, 69, 'test2', 58, #1990-02-12#);
INSERT INTO test (id, number, integer, date) VALUES (3, 420, 100, #1990-02-12#);

-- Union select
-- output:
-- | ID |  NUM |   STR |
-- |  2 | null |  null |
-- |  0 |   69 |  test |
-- |  4 |   69 | test1 |
-- |  5 |   69 | test2 |
-- |  3 |  420 |  null |
-- |  3 |  420 |  null |
-- |  1 | 2137 |  null |
SELECT id AS ID, number AS NUM, string AS STR FROM test ORDER BY number ASC UNION SELECT id AS ID, number AS NUM, string AS STR FROM test ORDER BY number DESC

-- Union all
-- output:
-- | ID |  NUM |   STR |
-- |  2 | null |  null |
-- |  0 |   69 |  test |
-- |  4 |   69 | test1 |
-- |  5 |   69 | test2 |
-- |  3 |  420 |  null |
-- |  3 |  420 |  null |
-- |  1 | 2137 |  null |
-- |  1 | 2137 |  null |
-- |  3 |  420 |  null |
-- |  3 |  420 |  null |
-- |  0 |   69 |  test |
-- |  4 |   69 | test1 |
-- |  5 |   69 | test2 |
-- |  2 | null |  null |
SELECT id AS ID, number AS NUM, string AS STR FROM test ORDER BY number ASC UNION ALL SELECT id AS ID, number AS NUM, string AS STR FROM test ORDER BY number DESC
