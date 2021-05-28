![CMake result](https://github.com/bergfi/dtree/actions/workflows/master.yml/badge.svg)

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

# License

Dtree - a concurrent compression tree for variable-length vectors
Copyright © 2018-2021 Freark van der Berg

Dtree is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Dtree is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Dtree.  If not, see <https://www.gnu.org/licenses/>.
