# Evidence ledger
## Binary-derived observations
docs/research-summary.md records factual identity and the narrow research scope. The full dossier, local metadata, decompiled excerpts and private research report remain locally retained under ignored evidence/rea/2026-10-26 and docs/gmp-backend-investigation.md; they are intentionally excluded from this commit. Recovered __gmpq_canonicalize is a compiled arithmetic-backend wrapper, not a complete symbolic engine. No original GMP routine was executed.

## Public implementation/documentation references
- https://gmplib.org/manual/Rational-Number-Functions — canonical rational contract; separate from native machine evidence.
- https://gmplib.org/manual/Copying — upstream distribution conditions; original DLL is not shipped here.
- https://docs.sympy.org/1.14.0/index.html — independent symbolic reference documentation.
- https://pypi.org/project/sympy/1.14.0/ — pinned reference package; isolated installation report retains hashes and URLs.
No upstream source file was copied into this engine. These consultations mean the work is not described as a source-blind clean-room exercise.

## Mathematical theory
Integer positional arithmetic, Euclidean GCD, rational normalization, precedence grammar, sum/product/quotient/integer-power derivative rules and rational-coefficient polynomial identities. Tests validate selected instances; no external reference establishes universal implementation correctness.

## Independent design choices
Base-10^9 signed vector representation, immutable shared expression nodes, bounded recursive descent, original-domain conditions, conservative total-expression checks, tri-state bounded polynomial checker, exception policy and CLI protocol. These choices differ from the recovered GMP layout. No proprietary Wolfram algorithms are claimed.

## Licensing/publication boundary
Original engine license has not been selected by the owner. GoogleTest is used from an explicitly supplied source directory with its existing notice. SymPy/mpmath reference packages retain their own licenses in .reference-python and are not redistributed. Raw original binaries are absent from this project. Select the original-work license and review excerpts/notices before publishing; this does not block private compilation/testing.
