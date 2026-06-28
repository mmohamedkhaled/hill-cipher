# Security Policy

## Supported versions

This project is educational and receives best-effort maintenance on the latest
`main` branch only.

## Reporting a vulnerability

If you discover a genuine security issue in the **project's own code** (e.g. a
buffer overflow, undefined behavior, or a path-handling flaw), please report it
**privately** rather than opening a public issue:

- Email: **opensource [at] mohamed-khaled [dot] dev**
- Or use GitHub's **"Report a vulnerability"** option on the **Security** tab.

Please include a reproduction and, if possible, a fix. I aim to acknowledge
reports within 72 hours.

## Scope — please read before reporting

**The Hill cipher itself is not a security mechanism.** It is a *classical*,
academically-broken cipher from 1929. This repository even ships an `attack`
subcommand that demonstrates how little plaintext is needed to recover the key.

Therefore the following are **out of scope** and will not be treated as
vulnerabilities:

- "The cipher can be broken" / "the key can be recovered" — by design.
- Ciphertext-only, known-plaintext, or chosen-plaintext cryptanalysis of the
  Hill cipher.
- Using this tool to protect real, sensitive data.

## Use guidance

Do **not** use this project to protect real communications or data. It exists
for education, experimentation, and as a reference implementation. Use modern,
peer-reviewed cryptography (e.g. AES-GCM, ChaCha20-Poly1305) for anything that
matters.
