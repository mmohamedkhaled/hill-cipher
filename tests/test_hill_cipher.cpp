// Unit tests for the Hill cipher library.
// No external framework: failures are reported to stderr and the process
// exits non-zero so `make test` / CI can detect them.

#include "hill_cipher.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__           \
                      << "  " #cond << '\n';                              \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

#define CHECK_THROWS(expr)                                                \
    do {                                                                  \
        bool thrown = false;                                              \
        try {                                                             \
            expr;                                                         \
        } catch (const std::exception&) {                                 \
            thrown = true;                                                \
        }                                                                 \
        CHECK(thrown);                                                    \
    } while (0)

using hillcipher::Matrix;
using hillcipher::Vector;

void test_char_conversion() {
    CHECK(hillcipher::char_to_index('a') == 0);
    CHECK(hillcipher::char_to_index('z') == 25);
    CHECK(hillcipher::char_to_index('A') == 0);   // upper-case folds to lower
    CHECK(hillcipher::char_to_index('Z') == 25);
    CHECK(hillcipher::char_to_index('1') == -1);  // non-alpha rejected
    CHECK(hillcipher::char_to_index(' ') == -1);

    CHECK(hillcipher::index_to_char(0) == 'a');
    CHECK(hillcipher::index_to_char(25) == 'z');
    CHECK(hillcipher::index_to_char(26) == 'a');  // wraps mod 26
    CHECK(hillcipher::index_to_char(-1) == 'z');  // negative wraps cleanly
}

void test_clean_and_pad() {
    CHECK(hillcipher::clean_and_pad("Ab C", 2) == "abcx");   // pad to even
    CHECK(hillcipher::clean_and_pad("Hello, World!", 3) == "helloworldxx");
    CHECK(hillcipher::clean_and_pad("abcd", 2) == "abcd");   // already aligned
    CHECK(hillcipher::clean_and_pad("", 2) == "");           // empty -> empty
    CHECK_THROWS(hillcipher::clean_and_pad("abc", 0));
}

void test_to_indices() {
    const Vector v = hillcipher::to_indices("abcd");
    CHECK((v == Vector{0, 1, 2, 3}));
}

void test_encrypt_identity_key() {
    // 2x2 identity key: ciphertext must equal plaintext.
    const Matrix id = {{1, 0}, {0, 1}};
    const Vector plain = {0, 1, 2, 3, 4, 5};
    const Vector cipher = hillcipher::encrypt(id, plain);
    CHECK((cipher == plain));
}

void test_encrypt_known_block() {
    // Key [[2,4],[3,5]], plaintext "no" -> "ef" (regression-validated).
    const Matrix key = {{2, 4}, {3, 5}};
    const Vector plain = {13, 14};  // n=13, o=14
    const Vector cipher = hillcipher::encrypt(key, plain);
    CHECK((cipher == Vector{4, 5}));  // e=4, f=5
}

void test_encrypt_modular_reduction() {
    // Key values large enough that the raw dot product exceeds 25; the result
    // must still be a valid index (this is the bug fixed by reducing mod 26
    // inside encrypt() rather than only at print time).
    const Matrix key = {{77, 52, 51, 43}, {55, 9, 31, 44}, {3, 18, 21, 45}, {15, 19, 71, 46}};
    const Vector plain = {7, 0, 11, 5};  // "half"
    const Vector cipher = hillcipher::encrypt(key, plain);
    for (int c : cipher) {
        CHECK(c >= 0 && c < 26);
    }
}

void test_encrypt_validation() {
    CHECK_THROWS(hillcipher::encrypt({}, {}));                 // dim 0 -> invalid
    CHECK_THROWS(hillcipher::encrypt({{1, 2, 3}}, {0, 1, 2})); // non-square row
    const Matrix good = {{1, 0}, {0, 1}};
    CHECK_THROWS(hillcipher::encrypt(good, {0, 1, 2}));        // length not a multiple
}

// --- Modular linear algebra & decryption -----------------------------------

void test_is_invertible() {
    // Identity is invertible (det = 1).
    CHECK(hillcipher::is_invertible({{1, 0}, {0, 1}}));
    // [[3,3],[2,5]] has det = 9, coprime with 26 -> invertible.
    CHECK(hillcipher::is_invertible({{3, 3}, {2, 5}}));
    // [[2,4],[3,5]] has det = -2 = 24 mod 26, gcd(24,26)=2 -> NOT invertible.
    CHECK(!hillcipher::is_invertible({{2, 4}, {3, 5}}));
    // Zero key is singular.
    CHECK(!hillcipher::is_invertible({{0, 0}, {0, 0}}));
}

void test_inverse_key() {
    // K^-1 of [[3,3],[2,5]] is [[15,17],[20,9]] (verified by hand).
    const Matrix key = {{3, 3}, {2, 5}};
    const Matrix inv = hillcipher::inverse_key(key);
    CHECK((inv == Matrix{{15, 17}, {20, 9}}));

    // Large entries are reduced mod 26 first: [[77,52],[55,9]] == [[25,0],[3,9]].
    // det = 25*9 - 0*3 = 225 = 225 mod 26 = 225 - 8*26 = 225-208 = 17.
    // 17^-1 mod 26: 17*23 = 391 = 15*26+1 = 391 -> 391 mod 26 = 1, so det_inv = 23.
    const Matrix inv2 = hillcipher::inverse_key({{77, 52}, {55, 9}});
    // K.K^-1 must be the identity mod 26.
    const Matrix check = hillcipher::inverse_key(inv2);  // inverse of inverse == original (mod 26)
    CHECK((check == Matrix{{25, 0}, {3, 9}}));
}

void test_decrypt_round_trip() {
    // Encrypting then decrypting with an invertible key yields the original.
    const Matrix key = {{3, 3}, {2, 5}};
    const Vector plain = hillcipher::to_indices("attackatdawngx");  // length 14, multiple of 2
    const Vector cipher = hillcipher::encrypt(key, plain);
    const Vector recovered = hillcipher::decrypt(key, cipher);
    CHECK((recovered == plain));
}

void test_decrypt_non_invertible_throws() {
    const Matrix key = {{2, 4}, {3, 5}};  // det = -2, not coprime with 26
    const Vector cipher = {4, 5, 4, 5};
    CHECK_THROWS(hillcipher::decrypt(key, cipher));
}

// --- Known-plaintext attack -------------------------------------------------

void test_recover_key_small() {
    // Recover a known 2x2 key from a plain/cipher pair generated with it.
    const Matrix key = {{3, 3}, {2, 5}};
    const std::string text = "thequickbrownfoxjumpsoverthelazydog";
    const std::string cleaned = hillcipher::clean_and_pad(text, 2);
    const Vector cipher = hillcipher::encrypt_text(key, text);
    std::string cipher_str;
    for (int idx : cipher) {
        cipher_str += hillcipher::index_to_char(idx);
    }
    const Matrix recovered = hillcipher::recover_key(cleaned, cipher_str, 2);
    CHECK((recovered == key));

    // Recovered key must re-encrypt the plaintext to the same ciphertext.
    const Vector recheck = hillcipher::encrypt(recovered, hillcipher::to_indices(cleaned));
    CHECK((recheck == cipher));
}

void test_recover_key_modulo_26() {
    // Keys are equivalent mod 26: the attack recovers the residue class.
    const Matrix key = {{77, 52, 51, 43}, {55, 9, 31, 44}, {3, 18, 21, 45}, {15, 19, 71, 46}};
    const std::string text =
        "halfacenturyagomitplayedacriticalroleinthedevelopmentoftheflightsoftware";
    const std::string cleaned = hillcipher::clean_and_pad(text, 4);
    const Vector cipher = hillcipher::encrypt_text(key, text);

    std::string cipher_str;
    for (int idx : cipher) cipher_str += hillcipher::index_to_char(idx);

    const Matrix recovered = hillcipher::recover_key(cleaned, cipher_str, 4);
    // Expected = original key reduced mod 26.
    const Matrix expected = {{25, 0, 25, 17}, {3, 9, 5, 18}, {3, 18, 21, 19}, {15, 19, 19, 20}};
    CHECK((recovered == expected));
}

void test_recover_key_validation() {
    CHECK_THROWS(hillcipher::recover_key("abc", "abc", 2));   // too short
    CHECK_THROWS(hillcipher::recover_key("ab", "abcd", 2));   // mismatched lengths
    CHECK_THROWS(hillcipher::recover_key("abcd", "abcd", 1)); // bad dimension
}

}  // namespace

int main() {
    test_char_conversion();
    test_clean_and_pad();
    test_to_indices();
    test_encrypt_identity_key();
    test_encrypt_known_block();
    test_encrypt_modular_reduction();
    test_encrypt_validation();
    test_is_invertible();
    test_inverse_key();
    test_decrypt_round_trip();
    test_decrypt_non_invertible_throws();
    test_recover_key_small();
    test_recover_key_modulo_26();
    test_recover_key_validation();

    if (g_failures == 0) {
        std::cout << "All unit tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " unit test(s) failed.\n";
    return 1;
}
