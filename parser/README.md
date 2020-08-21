build: rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -B debug && cmake --build debug
test:  ./debug/hp ./test/sample.hvml && echo yes
valgrind: valgrind --leak-check=full ./debug/hp ./test/sample.hvml && echo yes

