# Hill Cipher

[![CI](https://github.com/mmohamedkhaled/hill-cipher/actions/workflows/ci.yml/badge.svg)](https://github.com/mmohamedkhaled/hill-cipher/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](https://en.cppreference.com/w/cpp/17)

A compact, dependency-free C++17 implementation of the **[Hill cipher](https://en.wikipedia.org/wiki/Hill_cipher)**
&mdash; a classical polygraphic substitution cipher that encrypts blocks of
letters using linear algebra over &8484;&8322;&8326; (matrix multiplication mod 26).

Given an `n&times;n` key matrix `K`, each block of `n` plaintext letters is treated
as a column vector `p` and transformed into the ciphertext vector
`c = (K&middot;p) mod 26`. Decryption uses the modular inverse:
`p = (K&8315;&185;&middot;c) mod 26`.

The tool supports **encryption**, **decryption**, and a **known-plaintext
attack** that recovers the key.

---

## Table of Contents

- [Features](#features)
- [Prerequisites](#prerequisites)
- [Build](#build)
- [Usage](#usage)
- [How it works](#how-it-works)
- [Testing](#testing)
- [Project layout](#project-layout)
- [Sample cases](#sample-cases)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Security](#security)
- [License](#license)

---

## Features

- Pure C++17, no third-party libraries.
- **Encrypt, decrypt, and break** the cipher from one CLI.
- Decryption validates that the key is invertible mod 26 (via the extended
  Euclidean algorithm) and refuses non-invertible keys instead of producing
  garbage.
- **Known-plaintext attack** recovers the key from a matched plain/cipher pair
  and self-verifies by re-encryption.
- Hardened input handling: validates argument count, file existence, key
  dimensions (`n &isin; [2, 9]`), and matrix shape &mdash; no silent crashes.
- Modular arithmetic is applied **inside** the cipher math (not deferred to
  rendering), so the core routines are correct and reusable.
- Unit tests + a regression suite that checks program output byte-for-byte
  against saved baselines.
- Builds cleanly under `-Wall -Wextra -Wpedantic`.

## Prerequisites

- A C++17 compiler: `g++ >= 9`, `clang++ >= 10`, or MSVC (`cl` / Visual Studio 2019+).
- `make` (optional &mdash; you can also invoke the compiler directly).

## Build

```bash
git clone https://github.com/mmohamedkhaled/hill-cipher.git
cd hill-cipher
make
```

### With CMake (recommended on Windows / for IDEs)

```bash
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # optional: run the tests
```

<details>
<summary>Without a build system</summary>

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude \
    src/main.cpp src/io.cpp src/hill_cipher.cpp -o hillcipher
```
</details>

## Usage

```
hillcipher encrypt <keyfile> <plaintext>        Encrypt plaintext.
hillcipher decrypt <keyfile> <ciphertext>       Decrypt ciphertext.
hillcipher attack  <n> <plaintext> <ciphertext> Recover the n&times;n key.
hillcipher help                                 Show usage.
```

- `<keyfile>` &mdash; first line is the dimension `N`, followed by `N&times;N`
  integers (whitespace-separated).
- Text files &mdash; any 8-bit ASCII. Non-letters are ignored; letters are
  lower-cased and padded with `x` to a multiple of `N`.

### Encrypt

```bash
./hillcipher encrypt examples/keys/key_2x2.txt examples/plaintext/heisenberg.txt
```
```
Key matrix:
   2   4
   3   5

Plaintext:
notonlyistheuniversestrangerthanwethinkitisstrangerthanwecanthinkwernerheisenber
gx

Ciphertext:
efqxsqciitepovwzytawitizyrytooaniiooqlassteocmancmgqovktqwanooqlekytqhkioaawesyt
ad
```

### Decrypt

Decryption requires an **invertible** key &mdash; one whose determinant is
coprime with 26. The command prints the key, its modular inverse, the
ciphertext, and the recovered plaintext.

```bash
./hillcipher decrypt examples/keys/key_inv_2x2.txt examples/plaintext/heisenberg.txt
```
```
Key matrix:
   3   3
   2   5

Inverse key (mod 26):
  15  17
  20   9
...
```

> A key like `key_2x2.txt` (`[[2,4],[3,5]]`, det = &minus;2) is **not**
> invertible mod 26 and `decrypt` will refuse it with a clear error &mdash;
> that is a feature, not a bug.

### Known-plaintext attack

Given a plaintext and its matching ciphertext, recover the key:

```bash
./hillcipher attack 4 examples/plaintext/apollo.txt ciphertext.txt
```
```
Recovered key matrix:
  25   0  25  17
   3   9   5  18
   3  18  21  19
  15  19  19  20

Verification: re-encryption matches the ciphertext. PASS
```

> **Note:** the Hill cipher only depends on the key **mod 26**, so the attack
> recovers the key&rsquo;s residue class. For `apollo` the original key file
> contains entries like `77` and `52`; the recovered matrix shows `25` and `0`
> &mdash; the same key, reduced mod 26 &mdash; and it encrypts identically.

### Key file format

```
3
17 17  5
21 18 21
 2  2 19
```
Line 1 = dimension; the following `N` lines each contain `N` integers.

## How it works

- **Encrypt:** for each plaintext block `p`, compute `(K&middot;p) mod 26`.
- **Decrypt:** compute the modular inverse `K&8315;&185; = (det K)&8315;&185;&middot;adj(K) mod 26`
  using the [extended Euclidean algorithm](https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm)
  for the determinant&rsquo;s inverse, then `(K&8315;&185;&middot;c) mod 26`.
- **Attack:** from `n` plaintext/ciphertext column-vector pairs forming
  matrices `P`, `C` with `K&middot;P = C`, solve `K = C&middot;P&8315;&185; mod 26`,
  scanning for a plaintext window whose `P` is invertible.

## Testing

```bash
make test          # or, with CMake: ctest --test-dir build --output-on-failure
```

This runs:

1. **Unit tests** (`tests/test_hill_cipher.cpp`) &mdash; character conversion,
   padding, modular reduction, the modular inverse, decrypt round-trip, the
   non-invertible-key rejection, and key recovery.
2. **Regression suite** (`tests/regression.sh`) &mdash; encrypts every sample
   against its saved baseline, checks an encrypt&arr;decrypt round-trip, and
   runs the attack with built-in verification.

## Project layout

```
.
├── include/
│   ├── hill_cipher.hpp       # public crypto API (Doxygen-documented)
│   └── hill_cipher_io.hpp    # file/stdout I/O helpers (CLI layer)
├── src/
│   ├── hill_cipher.cpp       # cipher + modular linear algebra (pure, no I/O)
│   ├── io.cpp                # key-file reader + presentation helpers
│   └── main.cpp              # CLI: encrypt / decrypt / attack
├── tests/
│   ├── test_hill_cipher.cpp  # unit tests (no framework, no deps)
│   └── regression.sh         # baseline + round-trip + attack checks
├── examples/
│   ├── keys/                 # sample key matrices (incl. an invertible one)
│   ├── plaintext/            # sample plaintexts
│   └── expected/             # saved encryption baselines
├── .github/workflows/ci.yml  # make + cmake matrix, format check
├── Makefile                  # quick-start build (Linux/macOS)
├── CMakeLists.txt            # cross-platform build (Windows/IDEs)
├── .clang-format / .clang-tidy / .editorconfig
├── CONTRIBUTING.md / CODE_OF_CONDUCT.md / SECURITY.md
└── LICENSE
```

## Sample cases

| Key | Plaintext | Dimension | Notes |
|-----|-----------|-----------|-------|
| `key_identity_2x2.txt` | `alphabet.txt` | 2 | identity &mdash; sanity |
| `key_2x2.txt` | `heisenberg.txt` | 2 | non-invertible (det &minus;2) |
| `key_inv_2x2.txt` | (any) | 2 | invertible (det 9) &mdash; for decrypt demos |
| `key_4x4.txt` | `apollo.txt` | 4 | |
| `key_7x7.txt` | `enigma.txt` | 7 | |
| `key_9x9_sonnet.txt` | `sonnet27.txt` | 9 | |
| `key_9x9_neumann.txt` | `neumann.txt` | 9 | |

## Troubleshooting

| Symptom | Cause & fix |
|---------|-------------|
| `Error: value N is not invertible modulo 26 (gcd = ...)` from `decrypt` | The key's determinant is not coprime with 26, so it has no inverse. Use an invertible key (try `examples/keys/key_inv_2x2.txt`). |
| `Error: key dimension must be in [2, 9]` | The key file's first integer (dimension) is out of range, or a matrix row has the wrong number of columns. |
| `Error: cannot open file: ...` | The path does not exist or is unreadable. Check the argument order: `encrypt <keyfile> <plaintext>`. |
| `make: command not found` (Windows) | Windows has no `make` by default. Use the [CMake](#build) path instead. |
| `attack` prints `FAIL` | The recovered key did not reproduce the ciphertext &mdash; usually a mismatched plain/cipher pair or insufficient text (need at least `n*n` chars and an invertible window). |

## Contributing

Contributions are welcome &mdash; please read [CONTRIBUTING.md](CONTRIBUTING.md)
and the [Code of Conduct](CODE_OF_CONDUCT.md). Run `make test` before opening a
pull request.

## Security

The Hill cipher is a **classical**, academically-broken cipher &mdash; the
included `attack` command shows exactly how little plaintext an attacker needs
to recover the key. Do **not** use this project to protect real data. It exists
for education and experimentation. See [SECURITY.md](SECURITY.md) for the
reporting policy and scope.

## License

Released under the [MIT License](LICENSE) &copy; Mohamed Khaled.
