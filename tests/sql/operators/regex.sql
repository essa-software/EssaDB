-- output: 
-- |    a |
-- | true |
SELECT ('a' MATCH '[a-z]') AS a;

-- output: 
-- |     a |
-- | false |
SELECT ('A' MATCH '[a-z]') AS a;

-- error: Unexpected character within '[...]' in regular expression
SELECT ('A' MATCH '[a-z') AS a;
