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

-- error: Expected expression, got 'EOF'
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

-- error: Expected table or expression, got 'EOF'
SELECT COUNT(a) FROM

-- error: Expected table or expression, got 'EOF'
SELECT a, b, c FROM testa,

-- error: Expected 'JOIN' after INNER, got 'testb'
SELECT a, b, c FROM testa INNER testb

-- error: Expected 'OUTER' after 'FULL', got 'JOIN'
SELECT a, b, c FROM testa FULL JOIN testb ON a = b

-- error: Expected table or expression, got 'EOF'
SELECT a, b, c FROM testa RIGHT JOIN

-- error: Expected 'ON' expression after 'JOIN', got 'EOF'
SELECT a, b, c FROM testa LEFT JOIN testb

-- error: Expected '=' after column name, got 'b'
SELECT a, b, c FROM testa LEFT JOIN testb ON a b
