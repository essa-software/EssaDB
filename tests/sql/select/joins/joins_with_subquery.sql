IMPORT CSV 'tablea.csv' INTO tablea;
IMPORT CSV 'tableb.csv' INTO tableb;

-- Inner Join with subquery
-- output:
-- | id | a_string | a_number | b_string | b_number | id |
-- |  4 |    siema |       55 |      sql |       64 |  4 |
-- |  5 |    siema |       72 |       xd |       90 |  5 |
SELECT * FROM tablea INNER JOIN (SELECT TOP 3 * FROM tableb) ON tablea.id = tableb.id;

-- Inner Join with subquery vol2
-- output:
-- | b_string | b_number | id | id | a_string | a_number |
-- |      sql |       64 |  4 |  4 |    siema |       55 |
SELECT * FROM tableb INNER JOIN (SELECT TOP 2 * FROM tablea ORDER BY a_string DESC) ON tableb.id = tablea.id;
