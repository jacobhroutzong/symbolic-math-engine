# Validation — 2026-10-26
MSVC 19.44.35222, x64 Release, C++20; CMake 3.31.6; GoogleTest 1.17.0. Native CTest: **10/10 passed**, including nine GoogleTest cases and the optional independent-reference test. Raw configure/build/test logs and reference JSON are in evidence/local, excluded from publication.

Native tests cover:
- Multi-limb signed arithmetic, int64-min construction, quotient/remainder identities, normalization and division errors.
- 150 seeded algebraic integer fixtures, rational reduction and exact identities.
- Unary/power precedence, right associativity, signed power-base round trips, invalid syntax and resource limits.
- Partial-domain-preserving simplification, idempotence and exact evaluation consistency.
- Sum/product/quotient/power derivatives, multivariate inputs, original nonzero conditions and rejected unsupported powers.
- Structural inequality versus exact polynomial equivalence versus single-point agreement.
- LaTeX fractions/underscore escaping and symbol validation.

Initial tests caught doubled underscore escaping in LaTeX; it was corrected. Review also added signed-power-base serialization and x^1 differentiation regressions. Those checks pass in the final run.

Independent reference: tests/reference.py seed 20261026, 200 signed integers up to roughly 100 decimal digits compared with Python int, 100 normalized-rational calculations compared with Fraction, and 66 derivative/simplification cases compared with SymPy 1.14.0. Total **366 cases passed**. Integer quotient reference explicitly converts Python floor division to truncation toward zero.

SymPy cancellation checks equality on shared domains; separate conditions/native tests check domain restrictions. SymPy's convention for 0^0 is not adopted by this engine. No binary equivalence, universal algebraic correctness, industrial big-integer performance or complete CAS behavior is claimed. No performance benchmark was run for the symbolic core; accuracy/semantics were the milestone focus.

Only Windows/MSVC tested. Cross-platform compilation, sanitizers, much larger operands, fuzzing and a broader assumption/domain corpus remain next work. Exact arithmetic avoids rounding, not algorithmic mistakes or resource exhaustion.
