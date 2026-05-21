# lagoon

A lightweight LoongArch runtime assembler and disassembler library written in C.

Mnemonic functions are generated from [loongson-community/loongarch-opcodes](https://github.com/loongson-community/loongarch-opcodes). Documentation comments for LSX/LASX opcodes are extracted from [unofficial-loongarch-intrinsics-guide](https://github.com/jiegec/unofficial-loongarch-intrinsics-guide).

## Usage

### Assembler

```c
#include <lagoon.h>

uint8_t buffer[1024];
lagoon_assembler_t as;
la_init_assembler(&as, buffer, sizeof(buffer));

la_addi_d(&as, LA_A0, LA_ZERO, 42);
la_add_d(&as, LA_A0, LA_A0, LA_A1);
la_ret(&as);
```

### Labels

```c
#include <lagoon.h>

lagoon_label_t loop = {0};

la_bind(&as, &loop);
la_addi_d(&as, LA_A0, LA_A0, -1);
la_bnez(&as, LA_A0, la_label(&as, &loop));
la_label_free(&as, &loop);
```

### Disassembler

```c
#include <lagoon.h>

lagoon_insn_t insn;
la_disasm_one(0x02c00c04, &insn);

char buf[64];
la_insn_to_str(&insn, buf, sizeof(buf));
// buf = "addi.d $a0, $zero, 3"
```

## Build

```bash
# build the library
cmake -B build && cmake --build build

# install headers and the CMake package
cmake --install build --prefix /usr/local

# build a shared library instead
cmake -B build -DBUILD_SHARED_LIBS=ON && cmake --build build

# with tests
cmake -B build -DBUILD_TESTS=ON && cmake --build build
ctest --test-dir build

# Debug build with assertion on immediates enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

Or compile directly:

```bash
gcc -c lagoon.c -o lagoon.o
```

### CMake package

After installation, downstream projects can consume Lagoon with CMake:

```cmake
find_package(lagoon 1 CONFIG REQUIRED)

target_link_libraries(your_target PRIVATE lagoon::lagoon)
```

The package installs:
- `lagoon.h`
- the `lagoon::lagoon` target export
- `lagoonConfig.cmake` and `lagoonConfigVersion.cmake`

### Version macros

`lagoon.h` exposes public version macros for compile-time checks:

```c
#if LAGOON_VERSION >= LAGOON_VERSION_ENCODE(1, 0, 0)
/* use Lagoon 1.0.0+ APIs */
#endif
```

## Regenerating

After updating submodules:

```bash
git submodule update --init
./gen.py
```

This regenerates `include/lagoon.h` and `lagoon.c` from:
- `loongarch-opcodes` - instruction mnemonics and encodings
- `unofficial-loongarch-intrinsics-guide` - LSX/LASX documentation

## License

MIT
