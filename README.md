# Symbolic Mathematics Engine

## Phase 5 bounded exact linear equations

`solve_linear_equation(lhs,rhs,variable)` returns an exact rational root,
NoSolution, Identity on the original domain, or Unsupported. It retains
nonzero conditions, emits typed conditional-algebra trace events and verifies
candidate roots against both original ASTs. See
[scope, semantics and budgets](docs/phase5-linear-equations.md). The existing
parser, simplifier, differentiator and trace APIs remain unchanged; general
polynomial solving and rational factor cancellation are outside the solver’s scope.

The native and reference suite has 17 entries, including 15 GoogleTest cases
and 2 Python/SymPy reference tests. The equation reference covers 255 cases,
in addition to the 366 earlier reference fixtures. Configure
`SSM_REFERENCE_PYTHON` and optionally `SSM_REFERENCE_SITE` for an existing
SymPy1.14.0 environment. For example:

```sh
python tests/linear_equation_reference.py --probe build/Release/linear_equation_probe.exe --reference-path /path/to/site-packages
```

The equation solver is a native C++ API. The Phase 4 browser bundle provides
differentiation and reports its original domain restrictions.

The following sections retain earlier milestone validation snapshots.
Independent C++20 exact symbolic core, milestone 1, 2026-10-26. No C++ runtime dependencies beyond the standard library. This is independently developed mathematics, not proprietary Wolfram code or a reconstruction of GMP.

Implemented: dynamically sized signed integers (base 10^9), normalized rationals, immutable expression trees, precedence-aware parsing, deterministic round-trip printing, structural equality, conservative simplification, checked integer-power differentiation with original-domain conditions, bounded exact polynomial identity checking and LaTeX serialization.

## Build and test
CMake 3.24+, C++20 compiler. Provide an installed GoogleTest package or an existing source tree; configure never downloads dependencies.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSSM_GTEST_SOURCE=/path/to/googletest
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
Windows here uses the Visual Studio 17 2022 generator with -A x64 and fully qualified CMake/CTest from VS Build Tools. This run supplied GoogleTest 1.17.0 source already present in ScientificComputing/third_party; no engine links to the numerical project. Set BUILD_TESTING=OFF for a standalone library/CLI build with no GoogleTest.

Optional SymPy reference: configure SSM_REFERENCE_PYTHON to an existing Python. tests/reference.py accepts --reference-path for an isolated site-packages folder. This workspace installed pure-Python SymPy 1.14.0 and mpmath 1.3.0 from official PyPI into .reference-python; reference-install.json records download URLs and hashes. Nothing was installed globally. Reproduce with a separate environment or a project-local --target install, then:
```sh
python -B tests/reference.py --cli build/Release/symbolic_cli.exe --reference-path .reference-python --output evidence/local/reference.json
```

MSVC 19.44/x64 Release: 10/10 CTest entries passed (nine GoogleTest cases plus an independent reference test). Reference checked 200 large signed-integer cases, 100 rationals and 66 symbolic expressions. This is sampled exact mathematical validation, not original-binary differential validation or general correctness proof. See docs/semantics.md and docs/validation.md.

## API and CLI
Public headers include `integer.hpp`, `rational.hpp`, `expression.hpp`,
`trace.hpp`, and `linear_equation.hpp` under `include/ssm`.
The batch CLI reads three tab-separated fields per line and returns ok plus tab-separated results, or error plus a diagnostic:
```text
derive<TAB>(x+1)^3+2*x<TAB>x
eval<TAB>1/x<TAB>2
integer<TAB>123456789012345678901234567890<TAB>7
equiv<TAB>(x+1)^2<TAB>x^2+2*x+1
```
Replace <TAB> with a literal tab. derive returns simplified input, derivative, semicolon-separated nonzero conditions and LaTeX of the input. It is a local validation adapter, not an untrusted network service.

## Evidence and limitations
[REA arithmetic-backend summary](docs/research-summary.md) records a narrow compiled rational-canonicalization observation. GMP is an arithmetic backend, not a complete CAS. Our base-10^9 integers, parser, AST and derivatives are new designs. Binary observations, published references, theory and design choices remain separate.

Integration, transcendental functions, fractional or variable-power differentiation, general equivalence, and general rewrite proofs are outside the engine’s scope. The original DLL evidence comes from static analysis. The isolated reference environment retains the third-party package licenses.

For hash-pinned reference acquisition: `python -m pip install --only-binary=:all: --require-hashes --target .reference-python -r requirements-reference.txt`. The native engine does not require these packages. A separate BUILD_TESTING=OFF build of library and CLI also passed on this machine.

## Phase 4 browser release

See [browser build and limits](docs/browser.md). CI checks native C++ on Windows/Linux and builds a pinned Emscripten module on Linux. The public browser release uses the Phase 4 differentiation adapter. See the [release review](PUBLICATION.md).

## Checked transformation trace (native Phase 4)

See [trace scope and limits](docs/trace.md). The Phase 4 snapshot had 12 CTest entries, including 11 native tests and one independent Python reference test. Browser differentiation does not expose the native trace API.

## License and repository

[symbolic-math-engine](https://github.com/jacobhroutzong/symbolic-math-engine) — MIT. See [third-party notices](THIRD_PARTY_NOTICES.md). Original research binaries and raw decompilation are excluded.
