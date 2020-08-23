### shell commands ###
**build**:
```
rm -rf debug && cmake -B debug && cmake --build debug
```

**build with debug**:
```
rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -B debug && cmake --build debug
```

**build with verbose**: dumping compiling commands while executing
```
rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -B debug && cmake --build debug
```

**test**:
```
./debug/hp ./test/sample.hvml && echo yes
```

**test with ctest**:
```
pushd debug && ctest -vv; popd

```

**valgrind**: note: should have already been built with debug (verbose optionally)
```
valgrind --leak-check=full ./debug/hp ./test/sample.hvml && echo yes
```

### how to use test samples ###
```

1. put any test sample files (.hvml) in ./test
2. put its related output file (.hvml.output) in ./test
3. rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -B debug && cmake --build debug
4. pushd debug && ctest -vv; popd

```

