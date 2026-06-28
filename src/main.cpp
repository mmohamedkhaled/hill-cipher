/*============================================================================
| Program: Hill Cipher - encrypt, decrypt, and break (known-plaintext) the
|          classical Hill cipher over the 26-letter alphabet.
|
| Author:   Mohamed Khaled
| Language: C++17
| Build:    make
|           (or: g++ -std=c++17 -Iinclude src/main.cpp src/hill_cipher.cpp -o hillcipher)
|
| Usage:
|   hillcipher encrypt <keyfile> <plaintext>        Encrypt plaintext.
|   hillcipher decrypt <keyfile> <ciphertext>       Decrypt ciphertext.
|   hillcipher attack  <n> <plaintext> <ciphertext> Recover the n x n key.
|   hillcipher help                                 Show this help.
|
| Notes:
|   - <keyfile> line 1 = dimension N, followed by N*N integers.
|   - Decryption requires the key to be invertible mod 26 (det coprime to 26).
|   - The attack recovers the key from matched plain/cipher text and verifies
|     it by re-encrypting.
|   - Input is simple 8-bit ASCII; non-letters are ignored.
+===========================================================================*/

#include <iostream>
#include <string>

#include "hill_cipher.hpp"
#include "hill_cipher_io.hpp"

namespace {

void print_help(const char* program) {
    std::cout
        << "Usage:\n"
        << "  " << program << " encrypt <keyfile> <plaintext>\n"
        << "      Encrypt <plaintext> with the Hill cipher key in <keyfile>.\n"
        << "  " << program << " decrypt <keyfile> <ciphertext>\n"
        << "      Decrypt <ciphertext>. Requires an invertible key (mod 26).\n"
        << "  " << program << " attack <n> <plaintext> <ciphertext>\n"
        << "      Recover the n x n key from a matched plain/cipher pair.\n";
}

int run_encrypt(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: hillcipher encrypt <keyfile> <plaintext>\n";
        return 1;
    }
    const hillcipher::Matrix key = hillcipher::read_key_matrix(argv[2]);
    const std::string raw = hillcipher::read_file(argv[3]);
    const int n = static_cast<int>(key.size());
    const std::string cleaned = hillcipher::clean_and_pad(raw, n);
    const hillcipher::Vector plain = hillcipher::to_indices(cleaned);
    const hillcipher::Vector cipher = hillcipher::encrypt(key, plain);

    hillcipher::print_matrix("Key matrix", key);
    hillcipher::print_block("Plaintext", plain);
    hillcipher::print_block("Ciphertext", cipher);
    return 0;
}

int run_decrypt(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: hillcipher decrypt <keyfile> <ciphertext>\n";
        return 1;
    }
    const hillcipher::Matrix key = hillcipher::read_key_matrix(argv[2]);
    const std::string raw = hillcipher::read_file(argv[3]);
    const int n = static_cast<int>(key.size());
    const std::string cleaned = hillcipher::clean_and_pad(raw, n);
    const hillcipher::Vector cipher = hillcipher::to_indices(cleaned);
    const hillcipher::Vector plain = hillcipher::decrypt(key, cipher);  // validates invertibility

    hillcipher::print_matrix("Key matrix", key);
    hillcipher::print_matrix("Inverse key (mod 26)", hillcipher::inverse_key(key));
    hillcipher::print_block("Ciphertext", cipher);
    hillcipher::print_block("Plaintext", plain);
    return 0;
}

int run_attack(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: hillcipher attack <n> <plaintext> <ciphertext>\n";
        return 1;
    }
    const int n = std::stoi(argv[2]);
    const std::string plaintext = hillcipher::read_file(argv[3]);
    const std::string ciphertext = hillcipher::read_file(argv[4]);

    const hillcipher::Matrix recovered = hillcipher::recover_key(plaintext, ciphertext, n);
    hillcipher::print_matrix("Recovered key matrix", recovered);

    // Self-verify: encrypting the plaintext with the recovered key must
    // reproduce the ciphertext exactly.
    const std::string cleaned_plain = hillcipher::clean_and_pad(plaintext, n);
    const std::string cleaned_cipher = hillcipher::clean_and_pad(ciphertext, n);
    const hillcipher::Vector recomputed =
        hillcipher::encrypt(recovered, hillcipher::to_indices(cleaned_plain));
    const bool ok = (recomputed == hillcipher::to_indices(cleaned_cipher));

    std::cout << "\nVerification: "
              << (ok ? "re-encryption matches the ciphertext. PASS"
                     : "re-encryption does NOT match the ciphertext. FAIL")
              << '\n';
    return ok ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    const std::string command = argv[1];

    try {
        if (command == "encrypt") return run_encrypt(argc, argv);
        if (command == "decrypt") return run_decrypt(argc, argv);
        if (command == "attack") return run_attack(argc, argv);
        if (command == "help" || command == "--help" || command == "-h") {
            print_help(argv[0]);
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::cerr << "Unknown command: " << command << "\n\n";
    print_help(argv[0]);
    return 1;
}
