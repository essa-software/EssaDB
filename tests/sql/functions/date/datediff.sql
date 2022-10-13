CREATE TABLE events (start_date TIME, end_date TIME);

INSERT INTO events (start_date, end_date) VALUES(#2011-01-19#, #2011-03-27#);
INSERT INTO events (start_date, end_date) VALUES(#2010-11-17#, #2011-01-30#);
INSERT INTO events (start_date, end_date) VALUES(#2010-11-01#, #2010-09-17#);
INSERT INTO events (start_date, end_date) VALUES(#2011-02-26#, #2011-02-26#);
INSERT INTO events (start_date, end_date) VALUES(#2010-09-26#, #2010-11-19#);
INSERT INTO events (start_date, end_date) VALUES(#2010-08-11#, #2011-03-28#);
INSERT INTO events (start_date, end_date) VALUES(#2011-03-14#, #2010-07-13#);
INSERT INTO events (start_date, end_date) VALUES(#2010-12-17#, #2010-07-26#);
INSERT INTO events (start_date, end_date) VALUES(#2011-03-19#, #2010-08-03#);
INSERT INTO events (start_date, end_date) VALUES(#2010-10-01#, #2010-10-27#);

-- output:
-- | Diff |
-- |   67 |
-- |   74 |
-- |  -45 |
-- |    0 |
-- |   54 |
-- |  229 |
-- | -244 |
-- | -144 |
-- | -228 |
-- |   26 |
SELECT DATEDIFF(start_date, end_date) AS Diff FROM events;
