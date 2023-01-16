[Index](../Functions.md)

# Date/time functions

## `DATEDIFF`

Calculate time difference between arguments, in days.

* Arguments: `start`: time, `end`: time
* Returns: $(\textrm{T}(end)-\textrm{T}(start))/86400$, where $\textrm{T(n)}$ converts ISO-8601 date $n$ to its corresponding Unix timestamp in UTC timezone.

## `DAY`

Get day component of argument.

* Arguments: `time`: time
* Returns: `time`.day

## `MONTH`

Get month component of argument.

* Arguments: `time`: time
* Returns: `time`.month

## `YEAR`

Get year component of argument.

* Arguments: `time`: time
* Returns: `time`.year
