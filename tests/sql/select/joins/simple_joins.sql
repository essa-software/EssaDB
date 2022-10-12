IMPORT CSV 'tablea.csv' INTO tablea;
IMPORT CSV 'tableb.csv' INTO tableb;

-- Inner Join
-- output:
-- | id | a_string | a_number | b_string | b_number | id |
-- |  4 |    siema |       55 |      sql |       64 |  4 |
-- |  5 |    siema |       72 |       xd |       90 |  5 |
SELECT * FROM tablea INNER JOIN tableb ON tablea.id = tableb.id;

-- Left Join
-- output:
-- | id | a_string | a_number | b_string | b_number |   id |
-- |  1 |      abc |       55 |     null |     null | null |
-- |  2 |     test |       21 |     null |     null | null |
-- |  3 |      def |       64 |     null |     null | null |
-- |  4 |    siema |       55 |      sql |       64 |    4 |
-- |  5 |    siema |       72 |       xd |       90 |    5 |
SELECT * FROM tablea LEFT JOIN tableb ON tablea.id = tableb.id;

-- Right Join
-- FIXME: This skips id=6 for some reason
-- output:
-- |   id | a_string | a_number | b_string | b_number | id |
-- |    4 |    siema |       55 |      sql |       64 |  4 |
-- |    5 |    siema |       72 |       xd |       90 |  5 |
-- | null |     null |     null |    siema |       55 |  7 |
-- | null |     null |     null |      tej |       21 |  8 |
SELECT * FROM tablea RIGHT JOIN tableb ON tablea.id = tableb.id;

-- Outer Join
-- FIXME: This skips id=6 for some reason
-- output:
-- |   id | a_string | a_number | b_string | b_number |   id |
-- |    1 |      abc |       55 |     null |     null | null |
-- |    2 |     test |       21 |     null |     null | null |
-- |    3 |      def |       64 |     null |     null | null |
-- |    4 |    siema |       55 |      sql |       64 |    4 |
-- |    5 |    siema |       72 |       xd |       90 |    5 |
-- | null |     null |     null |    siema |       55 |    7 |
-- | null |     null |     null |      tej |       21 |    8 |
SELECT * FROM tablea FULL OUTER JOIN tableb ON tablea.id = tableb.id;
