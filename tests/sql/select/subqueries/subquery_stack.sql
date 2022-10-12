IMPORT CSV 'students.csv' INTO students;
IMPORT CSV 'data.csv' INTO data;

-- Basic
-- output:
-- | ID |               Name |        State |
-- |  1 |          Mike Kerr |    Minnesota |
-- |  2 | Marcelo Washington |      Wyoming |
-- |  3 |         Julie Dyer | Pennsylvania |
-- |  4 |        Royce Simon |     Virginia |
-- |  5 |       Tina Calhoun |      Georgia |
-- |  6 |      Jaclyn Kramer | North Dakota |
SELECT student_id AS ID, CONCAT(first_name, ' ', last_name) AS Name, (SELECT state FROM data d WHERE d.student_id = s.student_id) AS State FROM students s;

-- Correlation
-- output:
-- | ID |       Name |
-- |  1 |       Kerr |
-- |  2 | Washington |
-- |  3 |       Dyer |
-- |  4 |      Simon |
-- |  5 |    Calhoun |
-- |  6 |     Kramer |
SELECT student_id AS ID, (SELECT last_name FROM students s2 WHERE s1.student_id = s2.student_id) AS Name FROM students s1;
