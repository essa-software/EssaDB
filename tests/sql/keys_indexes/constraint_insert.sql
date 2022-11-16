CREATE TABLE customers (customer_id INT PRIMARY KEY, customer_name VARCHAR);

IMPORT CSV 'customers.csv' INTO customers;

CREATE TABLE orders (order_id INT PRIMARY KEY, customer_id INT FOREIGN KEY REFERENCES customers(customer_id), order_date TIME);

IMPORT CSV 'orders.csv' INTO orders;

-- error: Primary key must be unique
INSERT INTO orders (order_id, customer_id, order_date) VALUES(15, 0, #2022-01-01#);

-- error: Foreign key 'customer_id' requires matching value in referenced column 'customers.customer_id'
INSERT INTO orders (order_id, customer_id, order_date) VALUES(16, 2137, #2022-01-01#);
