[Index](../Functions.md)

# Math functions

## `ABS`

- Arguments: `x`: number
- Returns: $|x|$

## `ACOS`

- Arguments: `x`: number
- Returns: $\arccos(x)$, NaN for domain errors

## `ASIN`

- Arguments: `x`: number
- Returns: $\arcsin(x)$, NaN for domain errors

## `ATAN`

- Arguments: `x`: number
- Returns: $\arctg(x)$

## `ATN2`

- Arguments: `y`: number, `x`: number
- Returns: $\textrm{arctg2}(y, x)$ ([definition](https://en.wikipedia.org/wiki/Atan2#Definition_and_computation))

## `CEILING`

- Arguments: `x`: number
- Returns: $\lceil x \rceil$

## `COS`

- Arguments: `x`: number
- Returns: $\cos(x)$

## `COT`

- Arguments: `x`: number
- Returns: $\cot(x)$, NaN for domain errors

## `DEGREES`

Converts argument from radians to degrees.

- Arguments: `x`: number
- Returns: $\frac{180}{\pi} x$, NaN for domain errors

## `EXP`

- Arguments: `x`: number
- Returns: $e^x$

## `FLOOR`

- Arguments: `x`: number
- Returns: $\lfloor x \rfloor$

## `LOG`

- Arguments: `x`: number
- Returns: $\ln(x)$, NaN for domain errors

## `LOG10`

- Arguments: `x`: number
- Returns: $\log_{10}(x)$, NaN for domain errors

## `PI`

- Returns: $\pi$

## `POWER`

- Arguments: `a`: number, `b`: number
- Returns: $a^b$, NaN for domain errors

## `RAND`

Returns random value in system-defined range, using C `rand` function. This uses LCG and is not cryptographically secure.

- Returns: Random int in range `0` - `RAND_MAX`

## `ROUND`

Returns the integral value that is nearest to x, with halfway cases rounded away from zero.

- Arguments: `x`: number
- Returns: $\textrm{round}(x)$

## `SIGN`

- Arguments: `x`: number
- Returns: $
 \left\{
\begin{array}{c}
1\ \textrm{if}\ x > 0 \\
0\ \textrm{if}\ x = 0 \\
-1\ \textrm{if}\ x < 0
\end{array}
\right.$

## `SIN`

- Arguments: `x`: number
- Returns: $\sin(x)$

## `SQRT`

- Arguments: `x`: number
- Returns: $\sqrt{x}$, NaN for domain errors

## `SQUARE`

- Arguments: `x`: number
- Returns: $x^2$

## `TAN`

- Arguments: `x`: number
- Returns: $\tan(x)$, NaN for domain errors
