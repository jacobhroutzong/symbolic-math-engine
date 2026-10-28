# Third-party notices and source provenance

## Project source
All included implementation, adapters and tests are independently authored project code under MIT. Base10^9 integer arithmetic, rationals, parser/AST, derivatives and trace APIs do not embed/link GMP, SymPy, Wolfram or Octave implementations. The public research summary records factual GMP identifiers/observations only; raw decompilation is excluded. GMP's LGPL/GPL library terms do not license the project code, nor does MIT relicense GMP. No upstream implementation or documentation passages are bundled.

## External testing and toolchain dependencies

GoogleTest1.17.0 is fetched separately by CI at commit52eb8108c5bdec04579160ae17225d66034bd723. Its BSD-3-Clause license remains with that external checkout; it is not vendored in this source release. NumPy2.3.5 (numerical references) and SymPy1.14.0/mpmath1.3.0 (symbolic references) are separately acquired test tools, not engine runtime dependencies or bundled source. Preserve their notices if you distribute those dependencies yourself.

WebAssembly build uses Emscripten6.0.13. Generated binaries include permissively licensed runtime portions. The SDK's Emscripten LICENSE (MIT/NCSA and Node notices), musl COPYRIGHT, LLVM libc++/libc++abi/compiler-rt/libunwind LICENSE.TXT, and allocator notices are copied without truncation into the portfolio's runtime notice directory. No optional ports or original research DLLs are linked. Before distributing a different build, recheck its dependency/notice inventory.
