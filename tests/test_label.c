#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../include/lagoon.h"

typedef int64_t (*func64_t)(int64_t);

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

int main(void)
{
    int result = 0;

    result |= test_unbound_label_returns_zero();
    result |= test_bound_label_returns_offset();
    result |= test_forward_branch();
    result |= test_backward_branch();
    result |= test_multiple_forward_branches();

    if (result == 0) {
        printf("\nAll label tests passed!\n");
    } else {
        printf("\nSome label tests failed!\n");
    }

    return result;
}
