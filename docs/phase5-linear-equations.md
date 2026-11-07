# Phase 5 scope — bounded exact linear equations

Scope before implementation: solve LHS=RHS in one real variable with rational
coefficients. Reuse the immutable AST and exact Rational/Integer arithmetic.
Supported input operators are +,-,*,/, unary minus and direct integer powers
-4..4. No parameters, functions, inequalities, roots/complex numbers or systems.
Construct bounded numerator/denominator polynomials, retaining every original
nonzero denominator and nonpositive-power base condition. Clear denominators
only under those conditions. The resulting polynomial must have degree<=1;
otherwise return Unsupported, even if a more capable CAS could cancel factors.

Statuses distinguish a unique exact rational root, no solution, identity on
the original admissible domain, and unsupported scope/resource limits. Test a
candidate against every original condition and both original sides using exact
substitution; excluded roots are not returned. An identically zero required
denominator makes the original domain empty. Identities retain conditions
instead of expanding the domain (x/x=1, x^0=1, 0*(1/x)=0).

Limits:256 visited nodes across both sides, depth32, source coefficient128
decimal digits, retained coefficient numerator or denominator512 digits,
degree4 for every intermediate polynomial,4096 coefficient operations,64
conditions. These are explicit conservative budgets. Existing parser/API limits
remain unchanged. A single checked rational operation can temporarily exceed
the retained digit bound before rejection; exact candidate substitution also
uses the existing evaluator under the prevalidated tree/power bounds.
Unsupported is not NoSolution or a theorem-prover result.

Trace events distinguish original domain restrictions, conditional denominator
clearing, exact linear classification and exact substitution verification.
They are an aggregate algebraic certificate, not per-node rewrite proofs or a
general extension of the existing polynomial trace checker. Original ASTs,
simplifier, differentiator, browser adapter and Phase4 artifacts are preserved.

Independent reference: generated trusted grammar compared with SymPy's exact
solveset and rational arithmetic. Explicit domain-hole regressions supplement
SymPy because eager simplification can erase original syntactic holes.
https://docs.sympy.org/latest/modules/solvers/solveset.html is reference context,
not implementation or REA evidence. All new code is original MIT project code.
