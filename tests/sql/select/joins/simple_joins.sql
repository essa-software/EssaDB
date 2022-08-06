IMPORT CSV 'tablea.csv' INTO tablea;
IMPORT CSV 'tableb.csv' INTO tableb;

-- skip: Cross Join
-- output:
-- | tablea.cola | tablea.colb | tablea.colc | tableb.cola | tableb.colb | tableb.colc |
-- |           2 |        test |          21 |       siema |          55 |           7 |
-- |           2 |        test |          21 |        test |         102 |           6 |
-- |           2 |        test |          21 |          xd |          90 |           5 |
-- |           2 |        test |          21 |         tej |          21 |           8 |
-- |           2 |        test |          21 |         sql |          64 |           4 |
-- |           1 |         abc |          55 |       siema |          55 |           7 |
-- |           1 |         abc |          55 |        test |         102 |           6 |
-- |           1 |         abc |          55 |          xd |          90 |           5 |
-- |           1 |         abc |          55 |         tej |          21 |           8 |
-- |           1 |         abc |          55 |         sql |          64 |           4 |
-- |           3 |         def |          64 |       siema |          55 |           7 |
-- |           3 |         def |          64 |        test |         102 |           6 |
-- |           3 |         def |          64 |          xd |          90 |           5 |
-- |           3 |         def |          64 |         tej |          21 |           8 |
-- |           3 |         def |          64 |         sql |          64 |           4 |
-- |           5 |       siema |          72 |       siema |          55 |           7 |
-- |           5 |       siema |          72 |        test |         102 |           6 |
-- |           5 |       siema |          72 |          xd |          90 |           5 |
-- |           5 |       siema |          72 |         tej |          21 |           8 |
-- |           5 |       siema |          72 |         sql |          64 |           4 |
-- |           4 |       siema |          55 |       siema |          55 |           7 |
-- |           4 |       siema |          55 |        test |         102 |           6 |
-- |           4 |       siema |          55 |          xd |          90 |           5 |
-- |           4 |       siema |          55 |         tej |          21 |           8 |
-- |           4 |       siema |          55 |         sql |          64 |           4 |
SELECT * FROM tablea INNER JOIN tableb ON tablea.cola = tableb.colc;