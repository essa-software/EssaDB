CREATE TABLE test (id INT, [group] VARCHAR);
INSERT INTO test (id, [group]) VALUES(1, 'AA');
INSERT INTO test (id, [group]) VALUES(2, 'C');
INSERT INTO test (id, [group]) VALUES(3, 'B');
INSERT INTO test (id, [group]) VALUES(4, 'C');
INSERT INTO test (id, [group]) VALUES(null, 'AA');
INSERT INTO test (id, [group]) VALUES(5, 'C');
INSERT INTO test (id, [group]) VALUES(6, 'AA');
INSERT INTO test (id, [group]) VALUES(7, 'B');

CREATE TABLE new (id INT, [group] VARCHAR);
INSERT INTO new (id, [group]) VALUES(1, 'C');
INSERT INTO new (id, [group]) VALUES(2, 'AA');
INSERT INTO new (id, [group]) VALUES(3, 'AA');
INSERT INTO new (id, [group]) VALUES(4, 'B');
INSERT INTO new (id, [group]) VALUES(null, 'C');
INSERT INTO new (id, [group]) VALUES(5, 'B');
INSERT INTO new (id, [group]) VALUES(6, 'C');
INSERT INTO new (id, [group]) VALUES(7, 'C');

-- output:
-- |   id | group |
-- |    1 |    AA |
-- |    2 |     C |
-- |    3 |     B |
-- |    4 |     C |
-- | null |    AA |
-- |    5 |     C |
-- |    6 |    AA |
-- |    7 |     B |
SELECT * FROM test;

-- output:
-- |   id | group |
-- |    1 |     C |
-- |    2 |    AA |
-- |    3 |    AA |
-- |    4 |     B |
-- | null |     C |
-- |    5 |     B |
-- |    6 |     C |
-- |    7 |     C |
SELECT * FROM new;
