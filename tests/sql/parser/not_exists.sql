-- error: EXISTS existence condition cannot be used with CREATE TABLE
CREATE TABLE IF EXISTS test;

-- error: NOT EXISTS existence condition cannot be used with TRUNCATE TABLE
TRUNCATE TABLE IF NOT EXISTS test;

-- error: NOT EXISTS existence condition cannot be used with DROP TABLE
DROP TABLE IF NOT EXISTS test;
