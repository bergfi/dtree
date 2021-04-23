# Dtree

Dtree is a concurrent compression tree for storing vectors.

# Build

Dtree is a header-only library. To build the test, clone the repository and:

```
mkdir build && cd build
cmake .. -DDTREE_INCLUDE_TEST=1
make
```

See dtreetest.cpp for the options available.
