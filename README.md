[![Build Status](https://travis-ci.org/jimsch/cn-cbor.png?branch=master)](https://travis-ci.org/jimsch/cn-cbor)

# cn-cbor: A constrained node implementation of CBOR in C

This is a constrained node implementation of [CBOR](http://cbor.io) in
C that I threw together in 2013, before the publication of
[RFC 7049](http://tools.ietf.org/html/rfc7049), to validate certain
implementability considerations.

Its API model was inspired by
[nxjson](https://bitbucket.org/yarosla/nxjson).  It turns out that
this API model actually works even better with the advantages of the
CBOR format.

This code has been used in a number of research implementations on
constrained nodes, with resulting code sizes appreciably under 1 KiB
on ARM platforms.

I always meant to improve the interface some more with certain API
changes, in order to get even closer to 0.5 KiB, but I ran out of
time.  So here it is.  If I do get around to making these changes, the
API will indeed change a bit, so please be forewarned.

## Building

There is a `Simple-Makefile` for playing around, as well as a complete
[`cmake`](http://www.cmake.org)-based build environment.
(You can choose what fits your needs better.)

Building with `cmake`:

```sh
mkdir build
cd build
cmake ..
cmake --build . -- all
```

Generating a test coverage report (requires lcov[^1]; result in `build/lcov/index.html`):

```sh
rm -rf build
mkdir build
cd build
cmake .. -DCN_CBOR_COVERALLS_SEND=ON
cmake --build . -- all coveralls coverage_report
```

[^1]: Installation with homebrew: `brew install lcov`

## Consuming

### With cmake

find_package
```cmake
find_package(cn-cbor)
add_executable(myapp main.cpp)
target_link_libraries(myapp cn-cbor::cn-cbor)
```

add_subdirectory
```cmake
add_subdirectory(vendor/cn-cbor)
add_executable(myapp main.cpp)
target_link_libraries(myapp cn-cbor::cn-cbor)
```

### With Conan

This package is available in the [conan center](https://conan.io/center/cn-cbor/1.0.0)

add `cn-cbor/[>=1.0.0]` to you conan file

License: MIT
