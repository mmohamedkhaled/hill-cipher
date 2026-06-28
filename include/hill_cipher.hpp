#ifndef HILL_CIPHER_HPP
#define HILL_CIPHER_HPP

#include <string>
#include <vector>

namespace hillcipher {

/// A square matrix of integers used as a Hill cipher key.
using Matrix = std::vector<std::vector<int>>;
/// A column vector of letter indices (0..25).
using Vector = std::vector<int>;

/// Size of the cipher alphabet (a..z).
constexpr int kAlphabetSize = 26;
/// Smallest supported key dimension.
constexpr int kMinDimension = 2;
/// Largest supported key dimension.
constexpr int kMaxDimension = 9;

// ---------------------------------------------------------------------------
// Character <-> index conversion
// ---------------------------------------------------------------------------

/// Map a single character to a 0..25 index.
/// Lower- and upper-case letters are accepted; anything else returns -1.
int char_to_index(char c);

/// Map a 0..25 index back to 'a'..'z'. Values outside [0, 25] are wrapped
/// modulo 26 so the result is always a valid lower-case letter.
char index_to_char(int i);

// ---------------------------------------------------------------------------
// Text processing
// ---------------------------------------------------------------------------

/// Keep only alphabetic characters, lower-case them, and pad with 'x' until
/// the length is a multiple of `block_size`.
std::string clean_and_pad(const std::string& text, int block_size);

/// Convert a string of lower-case letters into a vector of indices.
Vector to_indices(const std::string& s);

// ---------------------------------------------------------------------------
// Encryption / decryption
// ---------------------------------------------------------------------------

/// Encrypt a vector of plaintext indices with the Hill cipher: each block of
/// `n` indices is multiplied by the `n x n` key matrix and reduced mod 26.
/// @throws std::invalid_argument if the key is not square, its dimension is
///         out of range [2, 9], or the plaintext length is not a multiple of n
Vector encrypt(const Matrix& key, const Vector& plaintext);

/// Encrypt raw text end-to-end (clean + pad + encrypt).
Vector encrypt_text(const Matrix& key, const std::string& text);

/// Decrypt a vector of ciphertext indices using the modular inverse of the
/// key: p = (K^-1 . c) mod 26.
/// @throws std::invalid_argument if the key is not invertible modulo 26, or
///         any of the dimension constraints of encrypt() are violated.
Vector decrypt(const Matrix& key, const Vector& ciphertext);

/// Decrypt raw text end-to-end.
Vector decrypt_text(const Matrix& key, const std::string& ciphertext);

// ---------------------------------------------------------------------------
// Modular linear algebra (key handling)
// ---------------------------------------------------------------------------

/// Compute the modular inverse of the key, K^-1 mod 26, using the determinant
/// and classical adjugate:
///   K^-1 = (det(K))^-1 . adj(K)   (all mod 26)
/// @throws std::invalid_argument if det(K) is not coprime with 26 (i.e. the
///         key is not invertible and decryption is impossible).
Matrix inverse_key(const Matrix& key);

/// Return true iff the key is invertible modulo 26 (det(K) coprime with 26).
bool is_invertible(const Matrix& key);

// ---------------------------------------------------------------------------
// Known-plaintext attack
// ---------------------------------------------------------------------------

/// Recover the n x n key matrix from a matched plaintext/ciphertext pair.
///
/// Given n column vectors of plaintext `p_j` and matching ciphertext `c_j`
/// with K . p_j = c_j (mod 26), this solves K = C . P^-1 (mod 26). It scans
/// the text for the first window of n consecutive blocks whose plaintext
/// matrix P is invertible mod 26.
///
/// @param plaintext  original plaintext (any case; non-letters ignored)
/// @param ciphertext the corresponding ciphertext (lower-case letters)
/// @param n          key dimension in [2, 9]
/// @return the recovered key matrix
/// @throws std::invalid_argument on bad dimension or mismatched lengths
/// @throws std::runtime_error if no invertible plaintext window exists
Matrix recover_key(const std::string& plaintext, const std::string& ciphertext, int n);

}  // namespace hillcipher

#endif  // HILL_CIPHER_HPP
