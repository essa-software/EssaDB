CREATE TABLE test (id INT, value INT DEFAULT 3000 CHECK value > 1000 CONSTRAINT id_check CHECK id != null CONSTRAINT value_check CHECK value != 3000);
ALTER TABLE test ALTER CHECK value != 3000, ALTER CONSTRAINT id_check CHECK value > 1000, ALTER CONSTRAINT value_check CHECK id NOT NULL;
-- error: Values doesn't match 'id_check' check rule specified for this table
INSERT INTO test (id, value) VALUES (1, 1000);
-- error: Values doesn't match general check rule specified for this table
INSERT INTO test (id) VALUES (2);
-- error: Values doesn't match 'value_check' check rule specified for this table
INSERT INTO test (value) VALUES (1002);
