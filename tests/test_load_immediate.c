#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../include/lagoon.h"

typedef int32_t (*func32_t)(void);
typedef int64_t (*func64_t)(void);

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

static int test_load_immediate32(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    struct {
        int32_t value;
        const char* description;
    } test_cases[] = {
        { 0, "zero" },
        { 1, "small positive (fits in 12 bits)" },
        { 0xfff, "max 12-bit unsigned" },
        { -1, "small negative (sign-extended 12 bits)" },
        { -2048, "min 12-bit signed" },
        { 2047, "max 12-bit signed" },
        { 0x1000, "just above 12 bits" },
        { 0x12345, "20-bit value with lower 12 bits zero" },
        { 0x123456, "full 24-bit value" },
        { 0x7fffffff, "max positive int32" },
        { -2147483648, "min negative int32" },
        { 0xdeadbeef, "arbitrary value 1" },
        { 0xcafebabe, "arbitrary value 2" },
        { 12345678, "arbitrary positive" },
        { -9876543, "arbitrary negative" },
    };

    lagoon_assembler_t assembler;
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        la_init_assembler(&assembler, buffer, buffer_size);

        la_load_immediate32(&assembler, LA_A0, test_cases[i].value);
        la_ret(&assembler);

        size_t code_size = assembler.cursor - assembler.buffer;
        __builtin___clear_cache(buffer, buffer + code_size);

        func32_t func = (func32_t)buffer;
        int32_t returned = func();

        if (returned != test_cases[i].value) {
            printf("FAIL: la_load_immediate32 %s: expected 0x%08x (%d), got 0x%08x (%d)\n",
                test_cases[i].description, test_cases[i].value, test_cases[i].value,
                returned, returned);
            result = 1;
        } else {
            printf("PASS: la_load_immediate32 %s: 0x%08x\n",
                test_cases[i].description, test_cases[i].value);
        }
    }

    free_executable_buffer(buffer, buffer_size);
    return result;
}

static int test_load_immediate64(void)
{
    int result = 0;
    size_t buffer_size = 1024;
    void* buffer = alloc_executable_buffer(buffer_size);
    if (!buffer) {
        printf("Failed to allocate executable buffer\n");
        return 1;
    }

    struct {
        int64_t value;
        const char* description;
    } test_cases[] = {
        { 0, "zero" },
        { 1, "small positive (fits in 12 bits)" },
        { 0xfff, "max 12-bit unsigned" },
        { -1, "small negative (sign-extended 12 bits)" },
        { -2048, "min 12-bit signed" },
        { 2047, "max 12-bit signed" },
        { 0x1000, "just above 12 bits" },
        { 0x12345, "20-bit value with lower 12 bits zero" },
        { 0x123456, "full 24-bit value" },
        { 0x7fffffff, "max positive int32" },
        { -2147483648LL, "min negative int32" },
        { 0xdeadbeef, "arbitrary int32 value 1" },
        { 0xcafebabe, "arbitrary int32 value 2" },
        { 0x100000000LL, "just above 32 bits" },
        { 0x123456789abcdef0LL, "full 64-bit value 1" },
        { 0xfedcba9876543210LL, "full 64-bit value 2" },
        { 0x7fffffffffffffffLL, "max positive int64" },
        { (int64_t)0x8000000000000000ULL, "min negative int64" },
        { 0x0000deadbeef0000LL, "value with middle bits set" },
        { 0xffff0000ffff0000LL, "alternating pattern" },
        { 0x123456789abc, "48-bit value" },
        { 0xffffffff12345678LL, "sign-extended from 32-bit with high bits" },
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        lagoon_assembler_t assembler;
        la_init_assembler(&assembler, buffer, buffer_size);

        la_load_immediate64(&assembler, LA_A0, test_cases[i].value);
        la_ret(&assembler);

        size_t code_size = assembler.cursor - assembler.buffer;
        __builtin___clear_cache(buffer, buffer + code_size);

        func64_t func = (func64_t)buffer;
        int64_t returned = func();

        if (returned != test_cases[i].value) {
            printf("FAIL: la_load_immediate64 %s: expected 0x%016lx (%ld), got 0x%016lx (%ld)\n",
                test_cases[i].description, test_cases[i].value, test_cases[i].value,
                returned, returned);
            result = 1;
        } else {
            printf("PASS: la_load_immediate64 %s: 0x%016lx\n",
                test_cases[i].description, test_cases[i].value);
        }
    }

    free_executable_buffer(buffer, buffer_size);
    return result;
}

int main(void)
{
    int result = 0;

    result |= test_load_immediate32();

    result |= test_load_immediate64();

    if (result == 0) {
        printf("\nAll tests passed!\n");
    } else {
        printf("\nSome tests failed!\n");
    }

    return result;
}
