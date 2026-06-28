#include "hill_cipher.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace hillcipher {

namespace {

/// Euclidean-friendly modulo: always returns a non-negative result.
long long mod(long long value, long long m) {
    const long long r = value % m;
    return r < 0 ? r + m : r;
}

/// Reduce every entry of a matrix modulo `m`.
Matrix mod_reduce(const Matrix& m, int m_mod) {
    Matrix out = m;
    for (auto& row : out) {
        for (auto& v : row) {
            v = mod(v, m_mod);
        }
    }
    return out;
}

/// Return a copy of `m` with row `r` and column `c` removed (a minor).
Matrix minor_matrix(const Matrix& m, int r, int c) {
    const int n = static_cast<int>(m.size());
    Matrix out;
    out.reserve(n - 1);
    for (int i = 0; i < n; ++i) {
        if (i == r) continue;
        std::vector<int> row;
        row.reserve(n - 1);
        for (int j = 0; j < n; ++j) {
            if (j == c) continue;
            row.push_back(m[i][j]);
        }
        out.push_back(std::move(row));
    }
    return out;
}

/// Exact integer determinant via cofactor expansion. `m` should already be
/// reduced mod 26 so intermediate values stay small. Uses 64-bit accumulation
/// to avoid overflow (a 9x9 cofactor product can reach ~25^9 > INT_MAX).
long long determinant(const Matrix& m) {
    const int n = static_cast<int>(m.size());
    if (n == 1) return m[0][0];
    if (n == 2) return static_cast<long long>(m[0][0]) * m[1][1] - static_cast<long long>(m[0][1]) * m[1][0];
    long long det = 0;
    for (int j = 0; j < n; ++j) {
        const long long sign = (j % 2 == 0) ? 1 : -1;
        det += sign * m[0][j] * determinant(minor_matrix(m, 0, j));
    }
    return det;
}

/// Classical adjugate (transpose of the cofactor matrix), with each entry
/// reduced mod 26 as it is produced so values stay small.
Matrix adjugate(const Matrix& m) {
    const int n = static_cast<int>(m.size());
    Matrix adj(n, Vector(n, 0));
    if (n == 1) {
        adj[0][0] = 1;
        return adj;
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const long long sign = ((i + j) % 2 == 0) ? 1 : -1;
            const long long cofactor = sign * determinant(minor_matrix(m, j, i));  // transposed
            adj[i][j] = static_cast<int>(mod(cofactor, kAlphabetSize));
        }
    }
    return adj;
}

/// Extended Euclidean algorithm: returns gcd(a, b) and sets x, y such that
/// a*x + b*y = gcd.
int extended_gcd(int a, int b, int& x, int& y) {
    if (b == 0) {
        x = 1;
        y = 0;
        return a;
    }
    int x1 = 0, y1 = 0;
    const int g = extended_gcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

/// Multiplicative inverse of `a` modulo `m`, or throw if it does not exist.
int mod_inverse(int a, int m) {
    a = mod(a, m);
    int x = 0, y = 0;
    const int g = extended_gcd(a, m, x, y);
    if (g != 1) {
        std::ostringstream msg;
        msg << "value " << a << " is not invertible modulo " << m
            << " (gcd = " << g << ")";
        throw std::invalid_argument(msg.str());
    }
    return mod(x, m);
}

/// Modular matrix inverse: K^-1 = (det(K))^-1 . adj(K)  (mod m).
/// Throws std::invalid_argument if the matrix is singular mod m.
Matrix inverse_mod(const Matrix& key_in, int m) {
    const Matrix key = mod_reduce(key_in, m);
    const int n = static_cast<int>(key.size());
    const int det = static_cast<int>(mod(determinant(key), m));
    const int det_inv = mod_inverse(det, m);  // throws if not invertible
    const Matrix adj = adjugate(key);
    Matrix inv(n, Vector(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            inv[i][j] = static_cast<int>(mod(det_inv * adj[i][j], m));
        }
    }
    return inv;
}

/// Standard matrix multiplication, reduced mod m.
Matrix matmul_mod(const Matrix& a, const Matrix& b, int m) {
    const int rows = static_cast<int>(a.size());
    const int inner = static_cast<int>(a.empty() ? 0 : a[0].size());
    const int cols = static_cast<int>(b.empty() ? 0 : b[0].size());
    Matrix result(rows, Vector(cols, 0));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int sum = 0;
            for (int k = 0; k < inner; ++k) {
                sum += a[i][k] * b[k][j];
            }
            result[i][j] = static_cast<int>(mod(sum, m));
        }
    }
    return result;
}

/// Apply a square matrix to consecutive length-n blocks of `input`, mod 26.
/// Shared by encrypt() and decrypt().
Vector apply_matrix(const Matrix& m, const Vector& input) {
    const int n = static_cast<int>(m.size());
    Vector out;
    out.reserve(input.size());
    for (size_t start = 0; start < input.size(); start += static_cast<size_t>(n)) {
        for (int i = 0; i < n; ++i) {
            int sum = 0;
            for (int j = 0; j < n; ++j) {
                sum += m[i][j] * input[start + static_cast<size_t>(j)];
            }
            out.push_back(mod(sum, kAlphabetSize));
        }
    }
    return out;
}

/// Validate that `key` is square with dimension in [2, 9].
void validate_key(const Matrix& key) {
    const int n = static_cast<int>(key.size());
    if (n < kMinDimension || n > kMaxDimension) {
        throw std::invalid_argument("key dimension must be in [2, 9]");
    }
    for (const auto& row : key) {
        if (static_cast<int>(row.size()) != n) {
            throw std::invalid_argument("key matrix must be square");
        }
    }
}

/// Validate that `v` is a multiple of `n` (non-empty).
void validate_block_alignment(const Vector& v, int n) {
    if (v.empty()) {
        throw std::invalid_argument("input must not be empty");
    }
    if (mod(static_cast<int>(v.size()), n) != 0) {
        throw std::invalid_argument("input length must be a multiple of the key dimension");
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Character / text helpers
// ---------------------------------------------------------------------------

int char_to_index(char c) {
    if (!std::isalpha(static_cast<unsigned char>(c))) {
        return -1;
    }
    return std::tolower(static_cast<unsigned char>(c)) - 'a';
}

char index_to_char(int i) {
    return static_cast<char>('a' + mod(i, kAlphabetSize));
}

std::string clean_and_pad(const std::string& text, int block_size) {
    if (block_size <= 0) {
        throw std::invalid_argument("block_size must be positive");
    }
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    while (mod(static_cast<int>(out.size()), block_size) != 0) {
        out += 'x';
    }
    return out;
}

Vector to_indices(const std::string& s) {
    Vector v;
    v.reserve(s.size());
    for (char c : s) {
        v.push_back(char_to_index(c));
    }
    return v;
}

// ---------------------------------------------------------------------------
// Encryption / decryption
// ---------------------------------------------------------------------------

Vector encrypt(const Matrix& key, const Vector& plaintext) {
    validate_key(key);
    validate_block_alignment(plaintext, static_cast<int>(key.size()));
    return apply_matrix(key, plaintext);
}

Vector encrypt_text(const Matrix& key, const std::string& text) {
    const std::string cleaned = clean_and_pad(text, static_cast<int>(key.size()));
    return encrypt(key, to_indices(cleaned));
}

Vector decrypt(const Matrix& key, const Vector& ciphertext) {
    validate_key(key);
    validate_block_alignment(ciphertext, static_cast<int>(key.size()));
    const Matrix inverse = inverse_mod(key, kAlphabetSize);  // throws if not invertible
    return apply_matrix(inverse, ciphertext);
}

Vector decrypt_text(const Matrix& key, const std::string& ciphertext) {
    const std::string cleaned = clean_and_pad(ciphertext, static_cast<int>(key.size()));
    return decrypt(key, to_indices(cleaned));
}

// ---------------------------------------------------------------------------
// Modular linear algebra (public)
// ---------------------------------------------------------------------------

Matrix inverse_key(const Matrix& key) {
    validate_key(key);
    return inverse_mod(key, kAlphabetSize);
}

bool is_invertible(const Matrix& key) {
    try {
        validate_key(key);
    } catch (const std::invalid_argument&) {
        return false;
    }
    // Invertible mod 26  <=>  gcd(det(K), 26) == 1.
    const Matrix reduced = mod_reduce(key, kAlphabetSize);
    const long long det = determinant(reduced);
    int x = 0, y = 0;
    return extended_gcd(static_cast<int>(mod(det, kAlphabetSize)), kAlphabetSize, x, y) == 1;
}

// ---------------------------------------------------------------------------
// Known-plaintext attack
// ---------------------------------------------------------------------------

Matrix recover_key(const std::string& plaintext, const std::string& ciphertext, int n) {
    if (n < kMinDimension || n > kMaxDimension) {
        throw std::invalid_argument("key dimension must be in [2, 9]");
    }

    const std::string p = clean_and_pad(plaintext, n);
    const std::string c = clean_and_pad(ciphertext, n);
    if (p.size() != c.size()) {
        throw std::invalid_argument("plaintext and ciphertext lengths differ after cleaning");
    }
    if (static_cast<int>(p.size()) < n * n) {
        throw std::invalid_argument("not enough data: need at least n*n characters");
    }

    const Vector pv = to_indices(p);
    const Vector cv = to_indices(c);
    const int num_blocks = static_cast<int>(p.size()) / n;

    // Slide a window of n consecutive blocks; recover K = C . P^-1 (mod 26)
    // from the first window whose plaintext matrix P is invertible.
    for (int s = 0; s + n <= num_blocks; ++s) {
        Matrix P(n, Vector(n, 0));
        Matrix C(n, Vector(n, 0));
        for (int j = 0; j < n; ++j) {          // column = block index
            for (int i = 0; i < n; ++i) {       // row within a block
                P[i][j] = pv[(s + j) * n + i];
                C[i][j] = cv[(s + j) * n + i];
            }
        }
        try {
            const Matrix P_inv = inverse_mod(P, kAlphabetSize);
            return mod_reduce(matmul_mod(C, P_inv, kAlphabetSize), kAlphabetSize);
        } catch (const std::invalid_argument&) {
            continue;  // P not invertible in this window; try the next
        }
    }
    throw std::runtime_error(
        "could not recover key: no invertible plaintext block window found");
}

}  // namespace hillcipher
