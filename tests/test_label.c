#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../include/lagoon.h"

typedef int64_t (*func64_t)(int64_t);
typedef uintptr_t (*funcptr_t)(void);

static int test_disassembler(uint32_t instruction, const char* expected_output, const char* description)
{
    lagoon_insn_t disasm_insn;
    char disasm_buf[256];

    la_disasm_one(instruction, &disasm_insn);
    la_insn_to_str(&disasm_insn, disasm_buf, sizeof(disasm_buf));

    if (strcmp(disasm_buf, expected_output)) {
        printf("FAIL: %s disasm\n", description);
        printf("Expected: '%s'\n", expected_output);
        printf("Got:      '%s'\n", disasm_buf);
        return 1;
    }

    printf("PASS: %s disasm: %s\n", description, disasm_buf);
    return 0;
}

static int test_la_local_disassembly(void* buffer, const char* description,
    const char* expected_pcalau12i, const char* expected_addi_d)
{
    int result = 0;
    uint8_t* code = (uint8_t*)buffer;

    result |= test_disassembler(*(uint32_t*)code, expected_pcalau12i, description);
    result |= test_disassembler(*(uint32_t*)(code + 4), expected_addi_d, description);
    return result;
}

static void* alloc_executable_buffer(size_t size)
{
    void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        return NULL;
    }
    return buffer;
}

static void free_executable_buffer(void* buffer, size_t size)
{
    munmap(buffer, size);
}

static int test_forward_branch(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    lagoon_assembler_t assembler;
    lagoon_label_t skip_label = { 0 };

    la_init_assembler(&assembler, buffer, buffer_size);

    la_beqz(&assembler, LA_A0, la_label(&assembler, &skip_label));
    la_ret(&assembler);
    la_bind(&assembler, &skip_label);
    la_load_immediate64(&assembler, LA_A0, 42);
    la_ret(&assembler);

    size_t code_size = assembler.cursor - assembler.buffer;
    __builtin___clear_cache(buffer, buffer + code_size);

    func64_t func = (func64_t)buffer;

    int64_t returned = func(100);
    if (returned != 100) {
        printf("FAIL: forward branch non-zero: expected 100, got %ld\n", returned);
        result = 1;
    } else {
        printf("PASS: forward branch non-zero: %ld\n", returned);
    }

    returned = func(0);
    if (returned != 42) {
        printf("FAIL: forward branch zero: expected 42, got %ld\n", returned);
        result = 1;
    } else {
        printf("PASS: forward branch zero: %ld\n", returned);
    }

    la_label_free(&assembler, &skip_label);
    free_executable_buffer(buffer, buffer_size);
    return result;
}

static int test_backward_branch(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    lagoon_assembler_t assembler;
    lagoon_label_t loop_label = { 0 };

    la_init_assembler(&assembler, buffer, buffer_size);

    la_load_immediate64(&assembler, LA_A1, 5);
    la_bind(&assembler, &loop_label);
    la_addi_d(&assembler, LA_A0, LA_A0, 1);
    la_addi_d(&assembler, LA_A1, LA_A1, -1);
    la_bnez(&assembler, LA_A1, la_label(&assembler, &loop_label));
    la_ret(&assembler);

    size_t code_size = assembler.cursor - assembler.buffer;
    __builtin___clear_cache(buffer, buffer + code_size);

    func64_t func = (func64_t)buffer;

    int64_t returned = func(0);
    if (returned != 5) {
        printf("FAIL: backward branch loop: expected 5, got %ld\n", returned);
        result = 1;
    } else {
        printf("PASS: backward branch loop: %ld\n", returned);
    }

    la_label_free(&assembler, &loop_label);
    free_executable_buffer(buffer, buffer_size);
    return result;
}

static int test_multiple_forward_branches(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    lagoon_assembler_t assembler;
    lagoon_label_t label = { 0 };

    la_init_assembler(&assembler, buffer, buffer_size);

    la_load_immediate64(&assembler, LA_A1, 0);
    la_beq(&assembler, LA_A0, LA_A1, la_label(&assembler, &label));
    la_addi_d(&assembler, LA_A1, LA_A1, 1);
    la_beq(&assembler, LA_A0, LA_A1, la_label(&assembler, &label));
    la_addi_d(&assembler, LA_A1, LA_A1, 1);
    la_beq(&assembler, LA_A0, LA_A1, la_label(&assembler, &label));
    la_load_immediate64(&assembler, LA_A0, 0);
    la_ret(&assembler);
    la_bind(&assembler, &label);
    la_load_immediate64(&assembler, LA_A0, 42);
    la_ret(&assembler);

    size_t code_size = assembler.cursor - assembler.buffer;
    __builtin___clear_cache(buffer, buffer + code_size);

    func64_t func = (func64_t)buffer;

    for (int i = 0; i <= 2; i++) {
        int64_t returned = func(i);
        if (returned != 42) {
            printf("FAIL: multiple branches case %d: expected 42, got %ld\n", i, returned);
            result = 1;
        } else {
            printf("PASS: multiple branches case %d: %ld\n", i, returned);
        }
    }

    int64_t returned = func(3);
    if (returned != 0) {
        printf("FAIL: multiple branches default case: expected 0, got %ld\n", returned);
        result = 1;
    } else {
        printf("PASS: multiple branches default case: %ld\n", returned);
    }

    la_label_free(&assembler, &label);
    free_executable_buffer(buffer, buffer_size);
    return result;
}

static int test_unbound_label_returns_zero(void)
{
    int result = 0;
    lagoon_assembler_t assembler;
    lagoon_label_t unbound_label = { 0 };
    uint8_t buffer[16];

    la_init_assembler(&assembler, buffer, sizeof(buffer));

    ptrdiff_t offset = la_label(&assembler, &unbound_label);
    if (offset != 0) {
        printf("FAIL: unbound label should return 0, got %ld\n", (long)offset);
        result = 1;
    } else {
        printf("PASS: unbound label returns 0\n");
    }

    if (unbound_label.offset_count != 1) {
        printf("FAIL: unbound label should have recorded 1 offset, got %zu\n", unbound_label.offset_count);
        result = 1;
    } else {
        printf("PASS: unbound label recorded offset count\n");
    }

    la_label_free(&assembler, &unbound_label);
    return result;
}

static int test_inline_label_offsets_grow_to_heap(void)
{
    int result = 0;
    lagoon_assembler_t assembler;
    lagoon_label_t label = { 0 };
    uint8_t buffer[128];
    size_t inline_capacity = sizeof(label.inline_offsets) / sizeof(label.inline_offsets[0]);

    la_init_assembler(&assembler, buffer, sizeof(buffer));

    for (size_t i = 0; i < inline_capacity; i++) {
        la_beqz(&assembler, LA_A0, la_label(&assembler, &label));
    }

    if (label.offset_count != inline_capacity) {
        printf("FAIL: inline offsets should record %zu offsets, got %zu\n",
            inline_capacity, label.offset_count);
        result = 1;
    } else {
        printf("PASS: inline offsets recorded offset count\n");
    }

    if (label.offset_capacity != inline_capacity) {
        printf("FAIL: inline offset capacity should be %zu, got %zu\n",
            inline_capacity, label.offset_capacity);
        result = 1;
    } else {
        printf("PASS: inline offsets kept inline capacity\n");
    }

    if (label.offsets != label.inline_offsets) {
        printf("FAIL: inline offsets should not allocate heap storage\n");
        result = 1;
    } else {
        printf("PASS: inline offsets use embedded storage\n");
    }

    la_beqz(&assembler, LA_A0, la_label(&assembler, &label));

    if (label.offset_count != inline_capacity + 1) {
        printf("FAIL: heap offsets should record %zu offsets, got %zu\n",
            inline_capacity + 1, label.offset_count);
        result = 1;
    } else {
        printf("PASS: heap offsets recorded offset count\n");
    }

    if (label.offset_capacity <= inline_capacity) {
        printf("FAIL: heap offset capacity should grow beyond %zu, got %zu\n",
            inline_capacity, label.offset_capacity);
        result = 1;
    } else {
        printf("PASS: heap offsets grew capacity\n");
    }

    if (label.offsets == label.inline_offsets || label.offsets == NULL) {
        printf("FAIL: heap offsets should move out of embedded storage\n");
        result = 1;
    } else {
        printf("PASS: heap offsets use allocated storage\n");
    }

    la_bind(&assembler, &label);
    la_label_free(&assembler, &label);
    return result;
}

static int test_bound_label_returns_offset(void)
{
    int result = 0;
    lagoon_assembler_t assembler;
    lagoon_label_t label = { 0 };
    uint8_t buffer[64];

    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bind(&assembler, &label);

    la_andi(&assembler, LA_ZERO, LA_ZERO, 0);
    la_andi(&assembler, LA_ZERO, LA_ZERO, 0);
    la_andi(&assembler, LA_ZERO, LA_ZERO, 0);

    ptrdiff_t offset = la_label(&assembler, &label);
    if (offset != -12) {
        printf("FAIL: bound label offset should be -12, got %ld\n", (long)offset);
        result = 1;
    } else {
        printf("PASS: bound label returns correct offset\n");
    }

    la_label_free(&assembler, &label);
    return result;
}

static int test_la_local_forward_label(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    lagoon_assembler_t assembler;
    lagoon_label_t label = { 0 };

    la_init_assembler(&assembler, buffer, buffer_size);

    la_la_local(&assembler, LA_A0, la_label(&assembler, &label));
    la_ret(&assembler);
    la_bind(&assembler, &label);

    result |= test_la_local_disassembly(buffer, "la.local forward label",
        "pcalau12i $a0, 0", "addi.d $a0, $a0, 12");

    uintptr_t expected = (uintptr_t)(assembler.buffer + label.location);
    size_t code_size = assembler.cursor - assembler.buffer;
    __builtin___clear_cache(buffer, buffer + code_size);

    funcptr_t func = (funcptr_t)buffer;
    uintptr_t returned = func();
    if (returned != expected) {
        printf("FAIL: la.local forward label: expected 0x%lx, got 0x%lx\n",
            (unsigned long)expected, (unsigned long)returned);
        result = 1;
    } else {
        printf("PASS: la.local forward label: 0x%lx\n", (unsigned long)returned);
    }

    la_label_free(&assembler, &label);
    free_executable_buffer(buffer, buffer_size);
    return result;
}

static int test_la_local_backward_label(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    lagoon_assembler_t assembler;
    lagoon_label_t label = { 0 };

    la_init_assembler(&assembler, buffer, buffer_size);

    la_bind(&assembler, &label);
    la_la_local(&assembler, LA_A0, la_label(&assembler, &label));
    la_ret(&assembler);

    result |= test_la_local_disassembly(buffer, "la.local backward label",
        "pcalau12i $a0, 0", "addi.d $a0, $a0, 0");

    uintptr_t expected = (uintptr_t)(assembler.buffer + label.location);
    size_t code_size = assembler.cursor - assembler.buffer;
    __builtin___clear_cache(buffer, buffer + code_size);

    funcptr_t func = (funcptr_t)buffer;
    uintptr_t returned = func();
    if (returned != expected) {
        printf("FAIL: la.local backward label: expected 0x%lx, got 0x%lx\n",
            (unsigned long)expected, (unsigned long)returned);
        result = 1;
    } else {
        printf("PASS: la.local backward label: 0x%lx\n", (unsigned long)returned);
    }

    la_label_free(&assembler, &label);
    free_executable_buffer(buffer, buffer_size);
    return result;
}

static int test_la_local_forward_label_with_signed_lo12(void)
{
    int result = 0;
    size_t buffer_size = 4096;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    lagoon_assembler_t assembler;
    lagoon_label_t label = { 0 };

    la_init_assembler(&assembler, buffer, buffer_size);

    la_la_local(&assembler, LA_A0, la_label(&assembler, &label));
    la_ret(&assembler);
    while ((size_t)(assembler.cursor - assembler.buffer) < 0x888) {
        la_nop(&assembler);
    }
    la_bind(&assembler, &label);

    result |= test_la_local_disassembly(buffer, "la.local forward label with signed lo12",
        "pcalau12i $a0, 1", "addi.d $a0, $a0, -1912");

    uintptr_t expected = (uintptr_t)(assembler.buffer + label.location);
    size_t code_size = assembler.cursor - assembler.buffer;
    __builtin___clear_cache(buffer, buffer + code_size);

    funcptr_t func = (funcptr_t)buffer;
    uintptr_t returned = func();
    if (returned != expected) {
        printf("FAIL: la.local forward label with signed lo12: expected 0x%lx, got 0x%lx\n",
            (unsigned long)expected, (unsigned long)returned);
        result = 1;
    } else {
        printf("PASS: la.local forward label with signed lo12: 0x%lx\n", (unsigned long)returned);
    }

    la_label_free(&assembler, &label);
    free_executable_buffer(buffer, buffer_size);
    return result;
}

int main(void)
{
    int result = 0;

    result |= test_unbound_label_returns_zero();
    result |= test_inline_label_offsets_grow_to_heap();
    result |= test_bound_label_returns_offset();
    result |= test_forward_branch();
    result |= test_backward_branch();
    result |= test_multiple_forward_branches();
    result |= test_la_local_forward_label();
    result |= test_la_local_backward_label();
    result |= test_la_local_forward_label_with_signed_lo12();

    if (result == 0) {
        printf("\nAll label tests passed!\n");
    } else {
        printf("\nSome label tests failed!\n");
    }

    return result;
}
