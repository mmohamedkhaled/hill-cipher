# Hill Cipher - build configuration
#
# Override the compiler or flags from the command line, e.g.:
#   make CXX=clang++ CXXFLAGS="-std=c++17 -O0 -g"

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDE  := -Iinclude
SRC      := src/hill_cipher.cpp src/io.cpp src/main.cpp
TEST_SRC := src/hill_cipher.cpp tests/test_hill_cipher.cpp
HDR      := include/hill_cipher.hpp include/hill_cipher_io.hpp

.PHONY: all test run check clean

all: hillcipher

hillcipher: $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(SRC) -o $@

test_hillcipher: $(TEST_SRC) $(HDR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_SRC) -o $@

## Run unit tests + the regression suite against the saved baselines.
test: hillcipher test_hillcipher
	./test_hillcipher
	bash tests/regression.sh ./hillcipher

## Quick smoke run on a sample case.
run: hillcipher
	./hillcipher encrypt examples/keys/key_2x2.txt examples/plaintext/heisenberg.txt

## Alias for `test` (common CI convention).
check: test

clean:
	rm -f hillcipher test_hillcipher *.o *.obj
