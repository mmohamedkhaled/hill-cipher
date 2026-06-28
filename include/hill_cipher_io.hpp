#ifndef HILL_CIPHER_IO_HPP
#define HILL_CIPHER_IO_HPP

#include <string>

#include "hill_cipher.hpp"

namespace hillcipher {

// ---------------------------------------------------------------------------
// I/O helpers
//
// These are presentation / file-reading utilities used by the CLI. They are
// kept out of the core cipher header so that a downstream user can embed the
// pure crypto library (hill_cipher.hpp) without pulling in <iostream>/<fstream>.
// ---------------------------------------------------------------------------

/// Read a key matrix from a file: first integer is dimension N, then N*N ints.
/// @throws std::runtime_error if the file cannot be opened or is malformed.
/// @throws std::invalid_argument if the dimension is outside [2, 9].
Matrix read_key_matrix(const std::string& path);

/// Read the entire contents of a file as a string.
/// @throws std::runtime_error if the file cannot be opened.
std::string read_file(const std::string& path);

/// Print a labelled matrix to stdout (4-wide, right-justified).
void print_matrix(const std::string& label, const Matrix& m);

/// Print a labelled block of indices as text, wrapping at 80 columns.
void print_block(const std::string& label, const Vector& data);

}  // namespace hillcipher

#endif  // HILL_CIPHER_IO_HPP
