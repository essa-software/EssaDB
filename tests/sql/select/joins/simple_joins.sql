IMPORT CSV 'tablea.csv' INTO tablea;
IMPORT CSV 'tableb.csv' INTO tableb;

-- Inner Join
-- output:
-- | tablea.cola | tablea.colb | tablea.colc | tableb.cola | tableb.colb | tableb.colc |
-- |           4 |       siema |          55 |         sql |          64 |           4 |
-- |           5 |       siema |          72 |          xd |          90 |           5 |
SELECT * FROM tablea INNER JOIN tableb ON tablea.cola = tableb.colc;

-- Left Join
-- output:
-- | tablea.cola | tablea.colb | tablea.colc | tableb.cola | tableb.colb | tableb.colc |
-- |           1 |         abc |          55 |        null |        null |        null |
-- |           2 |        test |          21 |        null |        null |        null |
-- |           3 |         def |          64 |        null |        null |        null |
-- |           4 |       siema |          55 |         sql |          64 |           4 |
-- |           5 |       siema |          72 |          xd |          90 |           5 |
SELECT * FROM tablea LEFT JOIN tableb ON tablea.cola = tableb.colc;

-- Right Join
-- output:
-- | tablea.cola | tablea.colb | tablea.colc | tableb.cola | tableb.colb | tableb.colc |
-- |         sql |          64 |           4 |           4 |       siema |          55 |
-- |          xd |          90 |           5 |           5 |       siema |          72 |
-- |       siema |          55 |           7 |        null |        null |        null |
-- |         tej |          21 |           8 |        null |        null |        null |
SELECT * FROM tablea RIGHT JOIN tableb ON tablea.cola = tableb.colc;

-- Outer Join
-- output:
-- | tablea.cola | tablea.colb | tablea.colc | tableb.cola | tableb.colb | tableb.colc |
-- |           1 |         abc |          55 |        null |        null |        null |
-- |           2 |        test |          21 |        null |        null |        null |
-- |           3 |         def |          64 |        null |        null |        null |
-- |           4 |       siema |          55 |         sql |          64 |           4 |
-- |           5 |       siema |          72 |          xd |          90 |           5 |
-- |       siema |          55 |           7 |        null |        null |        null |
-- |         tej |          21 |           8 |        null |        null |        null |
SELECT * FROM tablea FULL OUTER JOIN tableb ON tablea.cola = tableb.colc;