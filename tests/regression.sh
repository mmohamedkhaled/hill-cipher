#!/usr/bin/env bash
#
# Regression suite for the Hill cipher tool.
#
# 1. Encrypts every sample case and diffs against the saved baselines.
# 2. Round-trip check: encrypt then decrypt with an invertible key must
#    reproduce the original cleaned plaintext.
# 3. Known-plaintext attack: recover a key and confirm it re-encrypts the
#    ciphertext (the binary self-verifies and exits non-zero on failure).
#
# Usage: bash tests/regression.sh ./hillcipher

set -euo pipefail

BIN="${1:-./hillcipher}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

KEYS="$ROOT/examples/keys"
TEXT="$ROOT/examples/plaintext"
EXP="$ROOT/examples/expected"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ ! -x "$BIN" ]; then
  echo "error: binary not found or not executable: $BIN" >&2
  exit 2
fi

# Extract the body of a labelled section (e.g. "Ciphertext") from program
# output: the lines after "LABEL:" up to the next blank line.
extract_section() {
  awk -v L="$1:" '
    $0 == L { p = 1; next }
    p && /^$/ { exit }
    p { print }
  '
}

fails=0

echo "== Baseline encryption cases =="
# key | plaintext | expected-baseline
CASES=(
  "key_2x2.txt|heisenberg.txt|heisenberg.out"
  "key_4x4.txt|apollo.txt|apollo.out"
  "key_7x7.txt|enigma.txt|enigma.out"
  "key_9x9_sonnet.txt|sonnet27.txt|sonnet27.out"
  "key_9x9_neumann.txt|neumann.txt|neumann.out"
)
for entry in "${CASES[@]}"; do
  IFS='|' read -r key plain expected <<< "$entry"
  got="$("$BIN" encrypt "$KEYS/$key" "$TEXT/$plain")"
  want="$(cat "$EXP/$expected")"
  if [ "$got" == "$want" ]; then
    echo "PASS  encrypt $plain"
  else
    echo "FAIL  encrypt $plain"
    diff <(printf '%s' "$want") <(printf '%s' "$got") || true
    fails=$((fails + 1))
  fi
done

echo "== Decrypt round-trip (invertible key) =="
# Encrypt heisenberg with an invertible key, then decrypt; the plaintext
# sections of both runs must be identical.
ENC="$("$BIN" encrypt "$KEYS/key_inv_2x2.txt" "$TEXT/heisenberg.txt")"
enc_plain="$(printf '%s\n' "$ENC" | extract_section "Plaintext" | tr -d '\n')"
cipher="$(printf '%s\n' "$ENC" | extract_section "Ciphertext" | tr -d '\n')"
printf '%s' "$cipher" > "$TMP/cipher.txt"
DEC="$("$BIN" decrypt "$KEYS/key_inv_2x2.txt" "$TMP/cipher.txt")"
dec_plain="$(printf '%s\n' "$DEC" | extract_section "Plaintext" | tr -d '\n')"
if [ "$enc_plain" == "$dec_plain" ]; then
  echo "PASS  round-trip encrypt->decrypt"
else
  echo "FAIL  round-trip encrypt->decrypt"
  echo "  enc plain: $enc_plain"
  echo "  dec plain: $dec_plain"
  fails=$((fails + 1))
fi

echo "== Known-plaintext attack =="
# Recover the key for two dimensions; the binary self-verifies via exit code.
if "$BIN" attack 2 "$TEXT/heisenberg.txt" "$TMP/cipher.txt" >/dev/null 2>&1; then
  echo "PASS  attack recover key (n=2)"
else
  echo "FAIL  attack recover key (n=2)"
  fails=$((fails + 1))
fi
A_CIPHER="$("$BIN" encrypt "$KEYS/key_4x4.txt" "$TEXT/apollo.txt" \
            | extract_section "Ciphertext" | tr -d '\n')"
printf '%s' "$A_CIPHER" > "$TMP/apollo_cipher.txt"
if "$BIN" attack 4 "$TEXT/apollo.txt" "$TMP/apollo_cipher.txt" >/dev/null 2>&1; then
  echo "PASS  attack recover key (n=4)"
else
  echo "FAIL  attack recover key (n=4)"
  fails=$((fails + 1))
fi

echo "----------------------------------------"
if [ "$fails" -eq 0 ]; then
  echo "All regression cases passed."
  exit 0
fi
echo "$fails regression case(s) failed." >&2
exit 1
