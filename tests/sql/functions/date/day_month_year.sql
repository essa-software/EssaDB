CREATE TABLE events (date_ TIME);

INSERT INTO events (date_) VALUES(#1970-01-19#);
INSERT INTO events (date_) VALUES(#2010-11-17#);
INSERT INTO events (date_) VALUES(#2038-11-01#);
INSERT INTO events (date_) VALUES(#2100-02-26#);

-- output:
-- | Year | Month | Day |
-- | 1970 |     1 |  19 |
-- | 2010 |    11 |  17 |
-- | 2038 |    11 |   1 |
-- | 2100 |     2 |  26 |
SELECT YEAR(date_) AS Year, MONTH(date_) AS Month, DAY(date_) AS Day FROM events;
