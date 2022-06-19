IMPORT CSV 'where.csv' INTO test;

-- character_group
-- output:
-- | id | number | string | integer |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
SELECT * FROM test WHERE string LIKE 'test[12]';

-- character_range
-- output:
-- | id | number | string | integer |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
SELECT * FROM test WHERE string LIKE 'test[1-2]';

-- hash
-- output:
-- | id | number | string | integer |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
SELECT * FROM test WHERE string LIKE 'test#';

-- negated_character_group
-- output:
-- | id | number | string | integer |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string LIKE 'test[!1]';

-- negated_specified_char
-- output:
-- | id | number | string | integer |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string LIKE 'test[!1]';

-- prefix_asterisk
-- output:
-- | id | number | string | integer |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string LIKE '*st?';

-- question_mark
-- output:
-- | id | number | string | integer |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string LIKE 'test?';

-- suffix_asterisk_and_in
-- output:
-- | id | number | string |
-- |  5 |     69 |  test2 |
SELECT id, number, string FROM test WHERE string LIKE 'te*' AND id IN(1, 5);

-- suffix_asterisk
-- output:
-- | id | number | string | integer |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string LIKE 'te*';

-- two_asterisks
-- output:
-- | id | number | string | integer |
-- |  0 |     69 |   test |      48 |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
-- |  6 |   1234 |  testw |     165 |
SELECT * FROM test WHERE string LIKE '*st*';

-- asterisk_in_between
-- output:
-- | id | number | string | integer |
-- |  4 |     69 |  test1 |     122 |
-- |  5 |     69 |  test2 |      58 |
SELECT * FROM test WHERE string LIKE 't*t#';
