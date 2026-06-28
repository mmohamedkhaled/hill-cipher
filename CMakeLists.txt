# Hill Cipher - CMake build configuration.
#
# Minimal, dependency-free. Mirrors the Makefile but gives contributors on
# Windows (and IDE users) a one-command build:
#
#   cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
#
# The Makefile remains the canonical quick-start path on Linux/macOS.

cmake_minimum_required(VERSION 3.16)

project(hill_cipher
    VERSION 1.0.0
    DESCRIPTION "Dependency-free C++17 Hill cipher: encrypt, decrypt, known-plaintext attack"
    LANGUAGES CXX)

# --- Language & warning settings --------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# --- Core library (pure crypto, no I/O) --------------------------------------

add_library(hillcipher_core STATIC
    src/hill_cipher.cpp)
target_include_directories(hillcipher_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# --- CLI executable ----------------------------------------------------------

add_executable(hillcipher_cli src/main.cpp src/io.cpp)
target_link_libraries(hillcipher_cli PRIVATE hillcipher_core)
set_target_properties(hillcipher_cli PROPERTIES OUTPUT_NAME hillcipher)

# --- Tests -------------------------------------------------------------------

enable_testing()

# Unit tests: link only the core library (no I/O dependency).
add_executable(test_hill_cipher tests/test_hill_cipher.cpp)
target_link_libraries(test_hill_cipher PRIVATE hillcipher_core)
add_test(NAME unit_tests COMMAND test_hill_cipher)

# Regression suite: needs bash + the CLI binary.
find_program(BASH_EXECUTABLE bash)
if(BASH_EXECUTABLE)
    add_test(NAME regression
        COMMAND ${BASH_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/regression.sh
                $<TARGET_FILE:hillcipher_cli>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# --- Install (optional) ------------------------------------------------------

include(GNUInstallDirs)
install(TARGETS hillcipher_core hillcipher_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
