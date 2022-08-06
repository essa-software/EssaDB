IMPORT CSV 'tablea.csv' INTO tablea;
IMPORT CSV 'tableb.csv' INTO tableb;

-- Inner Join with subquery
-- output:
-- | tablea.cola | tablea.colb | tablea.colc | cola | colb | colc |
-- |           5 |       siema |          72 |   xd |   90 |    5 |
SELECT * FROM tablea INNER JOIN (SELECT TOP 3 * FROM tableb) ON tablea.cola = tableb.colc;

-- Inner Join with subquery vol2
-- output:
-- | tableb.cola | tableb.colb | tableb.colc | cola |  colb | colc |
-- |         sql |          64 |           4 |    4 | siema |   55 |
-- |          xd |          90 |           5 |    5 | siema |   72 |
SELECT * FROM tableb INNER JOIN (SELECT TOP 2 * FROM tablea ORDER BY cola DESC) ON tableb.colc = tablea.cola;