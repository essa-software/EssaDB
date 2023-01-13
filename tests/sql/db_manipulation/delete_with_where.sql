IMPORT CSV 'delete.csv' INTO test;

-- Basic deletion test
DELETE FROM test WHERE string LIKE 'te*';

-- output:
-- | string |     state |
-- |   null |      kept |
-- |   null |      kept |
-- |   null |      kept |
-- |   null | kept last |
SELECT * FROM test;

-- Inserting into freed rows - linked list test
INSERT INTO test (string, state) VALUES('test', 'removed 2');
INSERT INTO test (string, state) VALUES(NULL, 'inserted');
INSERT INTO test (string, state) VALUES('test', 'removed 2');
INSERT INTO test (string, state) VALUES(NULL, 'inserted');

-- output:
-- | string |     state |
-- |   null |      kept |
-- |   null |      kept |
-- |   null |      kept |
-- |   null | kept last |
-- |   test | removed 2 |
-- |   null |  inserted |
-- |   test | removed 2 |
-- |   null |  inserted |
SELECT * FROM test;

-- Deleting with first row in table not being first block in file
DELETE FROM test WHERE string LIKE 'te*';

-- output:
-- | string |     state |
-- |   null |      kept |
-- |   null |      kept |
-- |   null |      kept |
-- |   null | kept last |
-- |   null |  inserted |
-- |   null |  inserted |
SELECT * FROM test;

-- Deleting NULL varchars, iterating over "truncated" table
DELETE FROM test WHERE string IS NULL;

-- output:
-- Empty result set
SELECT * FROM test;
