IMPORT CSV 'tablea.csv' INTO tablea;
IMPORT CSV 'tableb.csv' INTO tableb;

-- error: Invalid column `nonexistent` used in join expression
SELECT * FROM tablea INNER JOIN tableb ON tablea.nonexistent = tableb.id;

-- error: Invalid column `nonexistent` used in join expression
SELECT * FROM tablea INNER JOIN tableb ON tablea.id = tableb.nonexistent;
