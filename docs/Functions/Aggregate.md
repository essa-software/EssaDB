[Index](../Functions.md)

# Aggregate

In all definitions, $R_n$ is a set of row in relation `R` mapped by expression `f` that are not null. Formally, 

$$R_n = \{f(r): r \isin R \land f(r)\ \text{is not null}\}.$$


## `AVG`

* Arguments: `R`: relation, `f`: expression
* Returns: Average of relation ($\overline{R_n}$)

## `COUNT`

* Arguments: `R`: relation, `f`: expression|`*`
* Returns:
    * If `f` is `*`, returns count of rows in `R` ($\#R$)
    * Otherwise, count of rows in relation ($\#R_n$).

## `MAX`

* Arguments: `R`: relation, `f`: expression
* Returns: $max(f(R_n))$

## `MIN`

* Arguments: `R`: relation, `f`: expression
* Returns: $min(f(R_n))$

## `SUM`

* Arguments: `R`: relation, `f`: expression
* Returns: $\sum f(R_n)$
