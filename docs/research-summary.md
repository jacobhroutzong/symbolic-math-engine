# Arithmetic-backend research summary

REA/Ghidra inspected the already installed Git distribution's GMP DLL as a narrow arithmetic-backend comparison. This project is independently implemented mathematics; it is not a complete CAS or a reconstruction of GMP's arithmetic kernels.

- Target: libgmp-10.dll, PE x86-64. Installed package metadata declares GMP 6.3.0-2; this is not signed-source equivalence proof.
- SHA-256: 37248cda33845e7c5c35a574e57da762dd0a8268f0270d179f30a33746809681.
- Function: __gmpq_canonicalize, analysis address 0x2599fa290.
- REA Evidence ID: ev_ea6e4c84c3bdb84a6b115ea09c966cc6b4b6dc2e94e326c9c8779d062750af2c.
- Static observations: denominator-sign adjustment, a zero-denominator call branch, a GCD call and conditional exact divisions. Kernel internals were not separately recovered.

The project's base-10^9 integer representation, parser, immutable AST, simplification and differentiation are new design choices. No original GMP routine was executed or differentially validated.

Raw REA records, decompiled excerpts, local package metadata and analysis logs are preserved privately outside the Git index. This summary contains factual identifiers and observations only; no original binary, assembly or decompiled implementation is redistributed.

Public context, separate from binary evidence:
- https://gmplib.org/manual/Rational-Number-Functions
- https://gmplib.org/manual/Copying

No proprietary Wolfram algorithms were recovered.
