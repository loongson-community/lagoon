# lagoon

A lightweight LoongArch runtime assembler and disassembler library written in C.

Mnemonic functions are generated from [loongson-community/loongarch-opcodes](https://github.com/loongson-community/loongarch-opcodes). Documentation comments for LSX/LASX opcodes are extracted from [unofficial-loongarch-intrinsics-guide](https://github.com/jiegec/unofficial-loongarch-intrinsics-guide).

## Usage

### Assembler

```c
#include "include/lagoon.h"

uint8_t buffer[1024];
lagoon_assembler_t as;
la_init_assembler(&as, buffer, sizeof(buffer));

la_addi_d(&as, LA_A0, LA_ZERO, 42);
la_add_d(&as, LA_A0, LA_A0, LA_A1);
la_ret(&as);
```

### Labels

```c
#include "include/lagoon.h"

lagoon_label_t loop = {0};

la_bind(&as, &loop);
la_addi_d(&as, LA_A0, LA_A0, -1);
la_bnez(&as, LA_A0, la_label(&as, &loop));
la_label_free(&as, &loop);
```

### Disassembler

```c
#include "include/lagoon.h"

lagoon_insn_t insn;
la_disasm_one(0x02c00c04, &insn);

char buf[64];
la_insn_to_str(&insn, buf, sizeof(buf));
// buf = "addi.d $a0, $zero, 3"
```

## Build

```bash
# as a static library
cmake -B build && cmake --build build

# with tests
cmake -B build -DBUILD_TESTS=ON && cmake --build build
ctest --test-dir build
```

Or compile directly:

```bash
gcc -c lagoon.c -o lagoon.o
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
