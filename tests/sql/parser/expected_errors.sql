-- error: Expected expression, got 'EOF'
SELECT

-- error: Expected expression, got 'FROM'
SELECT FROM test;

-- error: Expected expression, got 'EOF'
SELECT (

-- error: Expected expression, got ')'
SELECT )

-- error: Expected expression, got ')'
SELECT ()

-- error: Expected identifier in aggregate function, got 'EOF'
SELECT COUNT(

-- error: Expected expression, got 'EOF'
SELECT CHAR(

-- error: Expected ')' to close function, got 'EOF'
SELECT CHAR(a

-- error: Expected expression, got ','
SELECT CHAR(,

-- error: Expected expression, got 'EOF'
SELECT CHAR(a,

-- error: Expected expression, got ','
SELECT CHAR(a,,,,,a)

-- error: Expected ')' to close function, got 'FROM'
SELECT CHAR(a FROM test

-- error: Expected expression, got 'FROM'
SELECT CHAR(a, FROM test;

-- error: Expected identifier, got 'EOF'
SELECT COUNT(a) FROM

-- error: Expected identifier in aggregate function, got ')'
SELECT COUNT()
