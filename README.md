# Dtree

Dtree is a concurrent compression tree for storing vectors.

# Build

Clone the repository and then:

```
mkdir build && cd build
cmake .. -DWITH_LIBFRUGI=/path/to/libfrugi -DWITH_LTSMIN=/path/to/ltsmin/install/directory
```
- For libfrugi, see https://github.com/bergfi/libfrugi
- For LTSmin, see https://github.com/utwente-fmt/ltsmin
