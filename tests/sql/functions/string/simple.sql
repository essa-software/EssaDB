IMPORT CSV 'data.csv' INTO test;

-- len
-- output:
-- | id | STRING LEN |
-- |  0 |          4 |
-- |  1 |       null |
-- |  2 |       null |
-- |  3 |       null |
-- |  4 |          5 |
-- |  5 |          5 |
SELECT id, LEN(string) AS [STRING LEN] FROM test;

-- charindex
-- output:
-- | id | INDEX |
-- |  0 |     2 |
-- |  1 |  null |
-- |  2 |  null |
-- |  3 |  null |
-- |  4 |     2 |
-- |  5 |     2 |
SELECT id, CHARINDEX('st', string) AS [INDEX] FROM test;

-- char
-- output:
-- | id | CHAR |
-- |  0 |    0 |
-- |  1 |    A |
-- |  2 |    Y |
-- |  3 |    d |
-- |  4 |    z |
-- |  5 |    : |
SELECT id, CHAR(integer) AS [CHAR] FROM test;

-- concat
-- output
-- | id |        STRING |
-- |  0 |    test 69.05 |
-- |  1 | null 2137.123 |
-- |  2 |     null null |
-- |  3 |   null -420.5 |
-- |  4 |   test1 69.21 |
-- |  5 |   test2 69.45 |
SELECT id, CONCAT(string, ' ', STR(number)) AS [STRING] FROM test;

-- replace
-- output:
-- | id |    STRING |         REPLACED |
-- |  0 |  replaced |    1693   456    |
-- |  1 |      null |     169 34 56    |
-- |  2 |      null |   1 69  3   4    |
-- |  3 |      null |      1693 456    |
-- |  4 | replaced1 |         1693 456 |
-- |  5 | replaced2 |         1693 456 |
SELECT id, REPLACE(string, 'test', 'replaced') AS STRING, REPLACE([to_trim], '2', '69') AS [REPLACED] FROM test;

-- replicate
-- output:
-- | id |                STRING |
-- |  0 |    test, test, test,  |
-- |  1 |    null, null, null,  |
-- |  2 |    null, null, null,  |
-- |  3 |    null, null, null,  |
-- |  4 | test1, test1, test1,  |
-- |  5 | test2, test2, test2,  |
SELECT id, REPLICATE(CONCAT(string, ', '), 3) AS STRING FROM test;

-- reverse
-- output:
-- | id |     STRING |
-- |  0 |  654   321 |
-- |  1 |   65 43 21 |
-- |  2 | 4   3  2 1 |
-- |  3 |    654 321 |
-- |  4 |    654 321 |
-- |  5 |    654 321 |
SELECT id, REVERSE(TRIM([to_trim])) AS STRING FROM test;

-- left_right
-- output:
-- | id | LEFT | RIGHT |
-- |  0 | test |  test |
-- |  1 | null |  null |
-- |  2 | null |  null |
-- |  3 | null |  null |
-- |  4 | test |  est1 |
-- |  5 | test |  est2 |
SELECT id, LEFT(string, 4) AS LEFT, RIGHT(string, 4) AS RIGHT FROM test;

-- stuff
-- output:
-- | id |      REPLACED |
-- |  0 |  t replaced t |
-- |  1 |  n replaced l |
-- |  2 |  n replaced l |
-- |  3 |  n replaced l |
-- |  4 | t replaced t1 |
-- |  5 | t replaced t2 |
SELECT id, STUFF(string, 1, 2, ' replaced ') AS REPLACED FROM test;

-- translate
-- output:
-- | id |                                        TRANSLATED |
-- |  0 |  t TRANSLATION TRANSLATION WORDWORD TRANSLATION t |
-- |  1 |  n TRANSLATION TRANSLATION WORDWORD TRANSLATION l |
-- |  2 |  n TRANSLATION TRANSLATION WORDWORD TRANSLATION l |
-- |  3 |  n TRANSLATION TRANSLATION WORDWORD TRANSLATION l |
-- |  4 | t TRANSLATION TRANSLATION WORDWORD TRANSLATION t1 |
-- |  5 | t TRANSLATION TRANSLATION WORDWORD TRANSLATION t2 |
SELECT id, TRANSLATE(STUFF(string, 1, 2, ' WORD WORD WORDWORD WORD '), 'WORD', 'TRANSLATION') AS TRANSLATED FROM test;
