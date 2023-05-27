CREATE TABLE test (id INT, number FLOAT, string VARCHAR, integer INT, [to_trim] VARCHAR);
IMPORT CSV 'data.csv' INTO test;
IMPORT CSV 'data_asin.csv' INTO test_asin;

-- abs
-- output:
-- | id |         NUM | INT |
-- |  0 |   69.050003 |  48 |
-- |  1 | 2137.123047 |  65 |
-- |  2 |        null |  89 |
-- |  3 |  420.500000 | 100 |
-- |  4 |   69.209999 | 122 |
-- |  5 |    0.000000 |  58 |
SELECT id, ABS(number) AS [NUM], ABS(integer) AS [INT] FROM test;

-- acos
-- output:
-- | id |      NUM |
-- |  0 | 1.520775 |
-- |  1 | 1.355435 |
-- |  2 | 1.570796 |
-- |  3 | 2.037562 |
-- |  4 | 0.400157 |
-- |  5 | 2.498092 |
SELECT id, ACOS(number) AS [NUM] FROM test_asin;

-- asin
-- output:
-- | id |       NUM |
-- |  0 |  0.050021 |
-- |  1 |  0.215361 |
-- |  2 |  0.000000 |
-- |  3 | -0.466765 |
-- |  4 |  1.170640 |
-- |  5 | -0.927295 |
SELECT id, ASIN(number) AS [NUM] FROM test_asin;

-- atan
-- output:
-- | id |       NUM |       INT |
-- |  0 |  1.556315 |  1.549966 |
-- |  1 |  1.570328 |  1.555413 |
-- |  2 |  0.000000 |  1.559561 |
-- |  3 | -1.568418 | -1.560797 |
-- |  4 |  1.556349 |  1.562600 |
-- |  5 |  0.000000 |  1.553557 |
SELECT id, ATAN(number) AS [NUM], ATAN(integer) AS [INT] FROM test;

-- atn2
-- output:
-- | id |       NUM |       INT |
-- |  0 |  1.556315 |  1.549966 |
-- |  1 |  1.570328 |  1.555413 |
-- |  2 |  0.000000 |  1.559561 |
-- |  3 | -1.568418 | -1.560797 |
-- |  4 |  1.556349 |  1.562600 |
-- |  5 |  0.000000 |  1.553557 |
SELECT id, [ATN2](number, 1) AS [NUM], [ATN2](integer, 1) AS [INT] FROM test;

-- ceiling
-- output:
-- | id |  NUM |  INT |
-- |  0 |   70 |   48 |
-- |  1 | 2138 |   65 |
-- |  2 |    0 |   89 |
-- |  3 | -420 | -100 |
-- |  4 |   70 |  122 |
-- |  5 |    0 |   58 |
SELECT id, CEILING(number) AS [NUM], CEILING(integer) AS [INT] FROM test;

-- cos
-- output:
-- | id |      NUM |       INT |
-- |  0 | 0.997886 | -0.640144 |
-- |  1 | 0.667431 | -0.562454 |
-- |  2 | 1.000000 |  0.510177 |
-- |  3 | 0.890016 |  0.862319 |
-- |  4 | 0.995495 | -0.866767 |
-- |  5 | 1.000000 |  0.119180 |
SELECT id, COS(number) AS [NUM], COS(integer) AS [INT] FROM test;

-- cot
-- output:
-- | id |        NUM |       INT |
-- |  0 | -15.354574 |  0.833245 |
-- |  1 |   0.896276 | -0.680254 |
-- |  2 |        inf |  0.593181 |
-- |  3 |   1.952095 |  1.702957 |
-- |  4 |  10.498999 | -1.738007 |
-- |  5 |        inf |  0.120036 |
SELECT id, COT(number) AS [NUM], COT(integer) AS [INT] FROM test;

-- degrees
-- output:
-- | id |           NUM |          INT |
-- |  0 |   3956.273682 |  2750.197510 |
-- |  1 | 122448.132812 |  3724.225586 |
-- |  2 |      0.000000 |  5099.324219 |
-- |  3 | -24092.875000 | -5729.578125 |
-- |  4 |   3965.440918 |  6990.084961 |
-- |  5 |      0.000000 |  3323.155273 |
SELECT id, DEGREES(number) AS [NUM], DEGREES(integer) AS [INT] FROM test;

-- exp
-- output:
-- | id |                                    NUM |                                  INT |
-- |  0 |  972826309379214666687907364864.000000 |         701673558687019433984.000000 |
-- |  1 |                                    inf | 16948892066756750373006868480.000000 |
-- |  2 |                               1.000000 |                                  inf |
-- |  3 |                               0.000000 |                             0.000000 |
-- |  4 | 1141617741239628806689558364160.000000 |                                  inf |
-- |  5 |                               1.000000 |    15455388925836186341081088.000000 |
SELECT id, EXP(number) AS [NUM], EXP(integer) AS [INT] FROM test;

-- floor
-- output:
-- | id |  NUM |  INT |
-- |  0 |   69 |   48 |
-- |  1 | 2137 |   65 |
-- |  2 |    0 |   89 |
-- |  3 | -421 | -100 |
-- |  4 |   69 |  122 |
-- |  5 |    0 |   58 |
SELECT id, FLOOR(number) AS [NUM], FLOOR(integer) AS [INT] FROM test;

-- log
-- output:
-- | id |      NUM |      INT |
-- |  0 | 4.234831 | 3.871201 |
-- |  1 | 7.667216 | 4.174387 |
-- |  2 |     -inf | 4.488636 |
-- |  3 |     -nan |     -nan |
-- |  4 | 4.237145 | 4.804021 |
-- |  5 |     -inf | 4.060443 |
SELECT id, LOG(number) AS [NUM], LOG(integer) AS [INT] FROM test;

-- log10
-- output:
-- | id |      NUM |      INT |
-- |  0 | 1.839164 | 1.681241 |
-- |  1 | 3.329829 | 1.812913 |
-- |  2 |     -inf | 1.949390 |
-- |  3 |      nan |      nan |
-- |  4 | 1.840169 | 2.086360 |
-- |  5 |     -inf | 1.763428 |
SELECT id, [LOG10](number) AS [NUM], [LOG10](integer) AS [INT] FROM test;

-- power
-- output:
-- | id |            NUM |          INT |
-- |  0 |    4767.902832 |  2304.000000 |
-- |  1 | 4567295.000000 |  4225.000000 |
-- |  2 |       0.000000 |  7921.000000 |
-- |  3 |  176820.250000 | 10000.000000 |
-- |  4 |    4790.023926 | 14884.000000 |
-- |  5 |       0.000000 |  3364.000000 |
SELECT id, POWER(number, 2) AS [NUM], POWER(integer, 2) AS [INT] FROM test;

-- radians
-- output:
-- | id |       NUM |       INT |
-- |  0 |  1.205150 |  0.837758 |
-- |  1 | 37.299835 |  1.134464 |
-- |  2 |  0.000000 |  1.553343 |
-- |  3 | -7.339109 | -1.745329 |
-- |  4 |  1.207942 |  2.129302 |
-- |  5 |  0.000000 |  1.012291 |
SELECT id, RADIANS(number) AS [NUM], RADIANS(integer) AS [INT] FROM test;

-- round
-- output:
-- | id |  NUM |  INT |
-- |  0 |   69 |   48 |
-- |  1 | 2137 |   65 |
-- |  2 |    0 |   89 |
-- |  3 | -421 | -100 |
-- |  4 |   69 |  122 |
-- |  5 |    0 |   58 |
SELECT id, ROUND(number) AS [NUM], ROUND(integer) AS [INT] FROM test;

-- sign
-- output:
-- | id | NUM | INT |
-- |  0 |   1 |   1 |
-- |  1 |   1 |   1 |
-- |  2 |   0 |   1 |
-- |  3 |  -1 |  -1 |
-- |  4 |   1 |   1 |
-- |  5 |   0 |   1 |
SELECT id, SIGN(number) AS [NUM], SIGN(integer) AS [INT] FROM test;

-- sin
-- output:
-- | id |       NUM |       INT |
-- |  0 | -0.064989 | -0.768255 |
-- |  1 |  0.744671 |  0.826829 |
-- |  2 |  0.000000 |  0.860069 |
-- |  3 |  0.455929 |  0.506366 |
-- |  4 |  0.094818 |  0.498713 |
-- |  5 |  0.000000 |  0.992873 |
SELECT id, SIN(number) AS [NUM], SIN(integer) AS [INT] FROM test;

-- sqrt
-- output:
-- | id |       NUM |       INT |
-- |  0 |  8.309633 |  6.928203 |
-- |  1 | 46.229027 |  8.062258 |
-- |  2 |  0.000000 |  9.433981 |
-- |  3 |      -nan |      -nan |
-- |  4 |  8.319255 | 11.045361 |
-- |  5 |  0.000000 |  7.615773 |
SELECT id, SQRT(number) AS [NUM], SQRT(integer) AS [INT] FROM test;

-- square
-- output:
-- | id |            NUM |          INT |
-- |  0 |    4767.902832 |  2304.000000 |
-- |  1 | 4567295.000000 |  4225.000000 |
-- |  2 |       0.000000 |  7921.000000 |
-- |  3 |  176820.250000 | 10000.000000 |
-- |  4 |    4790.023926 | 14884.000000 |
-- |  5 |       0.000000 |  3364.000000 |
SELECT id, SQUARE(number) AS [NUM], SQUARE(integer) AS [INT] FROM test;

-- tan
-- output:
-- | id |       NUM |       INT |
-- |  0 | -0.065127 |  1.200127 |
-- |  1 |  1.115728 | -1.470038 |
-- |  2 |  0.000000 |  1.685825 |
-- |  3 |  0.512270 |  0.587214 |
-- |  4 |  0.095247 | -0.575372 |
-- |  5 |  0.000000 |  8.330857 |
SELECT id, TAN(number) AS [NUM], TAN(integer) AS [INT] FROM test;
