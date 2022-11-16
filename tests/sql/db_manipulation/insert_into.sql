CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT NOT NULL);

-- output:
-- null
INSERT INTO test (id, number, string, integer) VALUES(32, 5, 'test', -1);

-- columns not in order
-- output:
-- null
INSERT INTO test (id, integer, number, string) VALUES(32, 5, -1, 'test');

-- non-null must be specified
-- error: NULL given for NOT NULL column 'integer'
INSERT INTO test (id, number, string) VALUES(1, 1, 'test');

-- invalid types
-- error: Type mismatch, required INT but given VARCHAR for column 'id'
INSERT INTO test (id, number, string) VALUES('test', 1, 'test');
