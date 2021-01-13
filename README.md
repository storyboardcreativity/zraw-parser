# ZRAW-Parser

### How to build?

##### For Ubuntu:

0. Install all Nana C++ GUI dependencies

1. Clone Nana C++ GUI library
Example for v1.7.4: `git clone --depth 1 --branch v1.7.4 https://github.com/cnjinhao/nana`

2. Build Nana C++ GUI library
`cd nana/build/makefile/`
`make`

3. Copy `nana/build/bin/libnana.a` to `<ZRAW-Parser root folder>/lib/`

4. Copy `nana/include/nana/*` to `<ZRAW-Parser root folder>/include/nana/`

5. Type `make` and have fun!