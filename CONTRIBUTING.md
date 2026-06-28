# Contributing to Hill Cipher

Thanks for your interest in improving this project! This document explains how
to set up a development environment and submit changes.

## Quick start

```bash
git clone https://github.com/mmohamedkhaled/hill-cipher.git
cd hill-cipher
make          # build the CLI
make test     # unit tests + regression suite
```

Alternatively, with CMake (handy on Windows or inside an IDE):

```bash
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
```

You need a C++17 compiler (`g++ >= 9` or `clang++ >= 10`) and `make` **or**
CMake >= 3.16.

## Code style

- **Language standard:** C++17.
- **Naming:** `snake_case` for functions, variables, and files; `PascalCase`
  for types/classes. Constants use a `k` prefix (e.g. `kAlphabetSize`).
- **Headers:** never put `using namespace std;` in a header. Qualify fully in
  `.hpp` files.
- **Warnings:** code must compile with no warnings under
  `-Wall -Wextra -Wpedantic`.
- **Formatting:** enforced by `.clang-format` (4-space indent, attached braces).
  Run `clang-format -i src/*.cpp include/*.hpp tests/*.cpp` before committing.
  `.clang-tidy` is provided for optional local linting.
- **Documentation:** public API functions are documented with Doxygen-style
  comments in `include/hill_cipher.hpp` / `include/hill_cipher_io.hpp`. Keep
  them up to date.

## Architecture notes

- **Core logic** lives in `src/hill_cipher.cpp` and is strictly independent of
  I/O. It provides three operations: `encrypt`, `decrypt`, and `recover_key`
  (the known-plaintext attack), sharing one `apply_matrix` helper for the
  mod-26 block multiplication. Keep it free of `<iostream>` / `<fstream>`.
- **`src/io.cpp`** holds the file/stdout helpers (`read_key_matrix`,
  `read_file`, `print_matrix`, `print_block`), declared in
  `include/hill_cipher_io.hpp`. The unit tests deliberately link only the core,
  not `io.cpp`, to enforce the separation.
- **`src/main.cpp`** is a thin shell with `encrypt` / `decrypt` / `attack`
  subcommands: argument parsing and file I/O only.
- **Modular arithmetic** (`mod 26`) is performed inside `encrypt()` /
  `decrypt()` &mdash; do not defer it to rendering, or the math becomes
  incorrect for reuse.
- **Decryption** requires the key&rsquo;s modular inverse, computed as
  `(det K)&minus;1 &middot; adj(K)` via the extended Euclidean algorithm. A key
  whose determinant is not coprime with 26 is rejected with
  `std::invalid_argument`; preserve this check.
- **The key is only defined mod 26.** The attack returns the residue class of
  the key, which encrypts identically to the original &mdash; this is expected,
  not a bug.
- **Output format** (matrix in 4-wide columns, 80-column text wrapping) is a
  fixed contract: the regression baselines in `examples/expected/` depend on it.
  If you intentionally change the format, regenerate the baselines and explain
  why in your PR.

## Before opening a pull request

1. Create a branch off `main`: `git checkout -b feature/my-change`.
2. `make test` passes (unit + regression).
3. Add or update tests for any new behaviour.
4. Keep commits focused; write clear commit messages
   (e.g. `fix: apply mod 26 inside encrypt()`).
5. Ensure no compiler binaries (`*.exe`, `*.o`, the `hillcipher` executable)
   are committed &mdash; `.gitignore` already excludes them.

## Reporting bugs

Open an issue with:
- OS and compiler (`g++ --version`).
- Exact command that reproduces the problem.
- Expected vs. actual output.

## Licensing

By contributing, you agree your changes will be released under the project's
[MIT License](LICENSE).
