# Exact core semantics and boundaries
2026-10-26. Mathematical design choices, distinct from binary-derived evidence.

## Integers and rationals
Integer stores little-endian base-10^9 limbs and a separate sign. Zero has no limbs and no negative sign. Arithmetic is dynamically sized, not limited to int64; available memory/time still limit practical operands. Addition/subtraction compare magnitudes, multiplication is schoolbook, and long division binary-searches each quotient digit. Division truncates toward zero; remainder has the dividend's sign and satisfies a=q*b+r, |r|<|b|. Euclidean GCD uses nonnegative magnitudes, with gcd(0,0)=0.

Rational rejects zero denominators, moves a negative denominator sign to the numerator and divides both by their nonnegative GCD. Zero normalizes to 0/1. Multiplication cross-cancels factors before multiplying. Exact rational operations never use floating point. These algorithms were independently implemented; GMP's observed machine layout and allocation strategy were not copied.

## Expression grammar
```text
sum     := product (('+' | '-') product)*
product := unary (('*' | '/') unary)*
unary   := '+' unary | '-' unary | primary ('^' unary)?
primary := integer | identifier | '(' sum ')'
```
Powers associate right; multiplication/division and addition/subtraction associate left. Power binds above unary minus: -2^2=-4, (-2)^2=4, 2^-3=1/8. Identifiers use letters/underscores followed by letters/digits/underscores. Integers are decimal literals; fractions use explicit division. No implicit multiplication, decimal/scientific literals, functions or equality syntax.

Rational constant division and constant unary minus normalize at construction. Printing uses explicit parentheses and encloses negative integral constants so reparsing preserves structure even as a power base. “Canonical” here means deterministic structural serialization, not a universal canonical form for mathematical equivalence.

Parser byte offsets accompany syntax errors. Domain/unsupported-operation errors use MathError; integer construction/storage errors use standard exceptions. Input limit 4096 bytes, AST height and recursive operation limits 128. Public integer arithmetic itself has no fixed digit cap. Very large exact arithmetic can be slow; these are not complete denial-of-service controls.

## Domain-preserving transformations
Symbols range over exact rational values for evaluation. Missing bindings are errors. Division requires a nonzero denominator. Powers require integral exponents with magnitude at most 1024; zero to zero or a negative power is undefined in this engine. Exponent overflow/resource limits are errors, not approximate fallbacks.

Simplification folds exact constants and identities such as x+0, x*1, -(-x), x^1. It folds 0*x and x-x only if the removed expression is provably total in this limited language. It does not cancel x/x, 0/x, x^0 or 0*(1/x) blindly. simplify is deterministic and tested for idempotence on selected fixtures.

Differentiation supports constants, symbols, unary minus, sums, products, quotients and constant integer powers. It validates the differentiation variable and carries nonzero conditions from the original expression, including denominator expressions and bases of nonpositive powers. The result is a pair: derivative expression AND original-domain conditions. Consumers must enforce both. The x^1 rule returns the base derivative directly, avoiding an artificial x^0 hole. x^-1024 differentiation is explicitly rejected because the derivative exponent would exceed the supported limit. General x^x and fractional powers are unsupported.

There is no formal proof trace or symbolic assumptions solver. Conditions preserve the original supported rational-function domain; they are not a claim of differentiability for arbitrary analytic expressions.

## Equality and equivalence
structurally_equal compares operators, normalized constants, symbol names and ordered children.
polynomial_equivalence expands exact rational-coefficient polynomials into sorted monomial maps. It returns Equal, Different or Unknown. Limits: powers 1..16 after simplification, degree <=128, <=1024 terms and a shared operation budget. Nonconstant division/partial rational functions and out-of-scope expressions return Unknown. This checker does not use sampled values.

Evaluation agreement at selected points is weaker: x and x^2 agree at x=1 yet are distinct polynomials. Tests distinguish all three notions explicitly.

## Serialization and security
LaTeX emits controlled operators, exact fractions and escaped symbol underscores. It is a presentation serializer, not a typesetting engine. The web presentation uses existing KaTeX, not an independent scientific layout implementation. No source evaluation, subprocess launch or network access exists in the C++ engine.
