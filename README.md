# PurringCat

PurringCat is a reference implementation of HVML.

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
./debug/test/parser/hp ./test/parser/test/sample.hvml && echo yes
```

**test with ctest**:
```
pushd debug/test/parser && ctest -vv; popd

```

**valgrind**: note: should have already been built with debug (verbose optionally)
```
valgrind --leak-check=full ./debug/test/parser/hp ./test/parser/test/sample.hvml && echo yes
```

### how to use test samples ###
```

1. put any test sample files (.hvml) in ./test/parser/test
2. put its related output file (.hvml.output) in ./test/parser/test
3. rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -B debug && cmake --build debug
4. pushd debug/test/parser && ctest -VV; popd

```

