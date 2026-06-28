#include "hill_cipher_io.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace hillcipher {

// ---------------------------------------------------------------------------
// File reading
// ---------------------------------------------------------------------------

Matrix read_key_matrix(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        throw std::runtime_error("cannot open key file: " + path);
    }

    int n = 0;
    if (!(f >> n)) {
        throw std::runtime_error("invalid key file: missing dimension");
    }
    if (n < kMinDimension || n > kMaxDimension) {
        std::ostringstream msg;
        msg << "key dimension must be in [" << kMinDimension << ", " << kMaxDimension
            << "], got " << n;
        throw std::invalid_argument(msg.str());
    }

    Matrix key(n, Vector(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (!(f >> key[i][j])) {
                throw std::runtime_error("invalid key matrix data");
            }
        }
    }
    return key;
}

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("cannot open file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

// ---------------------------------------------------------------------------
// Presentation
// ---------------------------------------------------------------------------

void print_matrix(const std::string& label, const Matrix& m) {
    std::cout << '\n' << label << ":\n";
    for (const auto& row : m) {
        for (int value : row) {
            std::cout << std::setw(4) << value;
        }
        std::cout << '\n';
    }
}

void print_block(const std::string& label, const Vector& data) {
    std::cout << '\n' << label << ":\n";
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << index_to_char(data[i]);
        if ((i + 1) % 80 == 0) {
            std::cout << '\n';
        }
    }
    std::cout << '\n';
}

}  // namespace hillcipher
