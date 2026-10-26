# Publication readiness

Local commit only. No remote repository or visibility has been selected. Do not publish until the owner approves an original-work license, repository name and intended public/private visibility. No open-source license is implied by preparing a Git commit.

Included: original C++20 source, public headers, CMake build, automated tests, CLI adapter, hash-pinned optional reference requirements, build/semantics/validation documentation and a factual research summary.

Excluded: build products, executable/DLL/archive files, optional SymPy/mpmath installation, raw REA dossiers and bundles, decompiled excerpts, local analysis logs, personal paths, datasets and machine configuration. Existing ignored originals are preserved on disk.

Dependencies: native core uses the C++ standard library. GoogleTest is supplied externally for tests; its source/notices are not vendored. Optional SymPy/mpmath references are separately acquired from PyPI with pinned wheel hashes and retain their own licenses. The engine does not link to or redistribute GMP.

Validation: 10/10 CTest entries passed on Windows/MSVC, including 366 independent reference cases; standalone BUILD_TESTING=OFF builds passed. Cross-platform CI and sanitizers remain unexecuted. Public source release awaits the decisions above.
