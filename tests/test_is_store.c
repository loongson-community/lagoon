#include "lagoon.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int test_store(lagoon_assembler_t* asm_, const char* name, bool expected)
{
    uint32_t insn = *(uint32_t*)asm_->buffer;
    bool result = la_is_store_instruction(insn);
    if (result != expected) {
        printf("FAIL: %s - expected %d, got %d (insn=0x%08X)\n", name, expected, result, insn);
        return 1;
    }
    printf("PASS: %s\n", name);
    return 0;
}

int main(void)
{
    int failures = 0;
    uint8_t buffer[16];
    lagoon_assembler_t asm_;

    // ====== POSITIVE TESTS: Store instructions ======

    // st.{b,h,w,d} and stptr.{b,h,w,d} - opcode 0x29, 0x2D
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_st_b(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "st.b", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_st_h(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "st.h", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_st_w(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "st.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_st_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "st.d", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stptr_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "stptr.d", true);

    // stx.{b,h,w,d}
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stx_b(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stx.b", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stx_h(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stx.h", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stx_w(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stx.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stx_d(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stx.d", true);

    // fst.{s,d}
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fst_s(&asm_, LA_FA0, LA_A1, 0);
    failures += test_store(&asm_, "fst.s", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fst_d(&asm_, LA_FA0, LA_A1, 0);
    failures += test_store(&asm_, "fst.d", true);

    // fstx.{s,d}
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fstx_s(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fstx.s", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fstx_d(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fstx.d", true);

    // vst, vstx
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vst(&asm_, LA_V0, LA_A1, 0);
    failures += test_store(&asm_, "vst", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vstx(&asm_, LA_V0, LA_A1, LA_A2);
    failures += test_store(&asm_, "vstx", true);

    // xvst, xvstx
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvst(&asm_, LA_XV0, LA_A1, 0);
    failures += test_store(&asm_, "xvst", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvstx(&asm_, LA_XV0, LA_A1, LA_A2);
    failures += test_store(&asm_, "xvstx", true);

    // stgt.{b,h,w,d}, stle.{b,h,w,d}
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stgt_b(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stgt.b", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stgt_h(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stgt.h", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stgt_w(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stgt.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stgt_d(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stgt.d", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stle_b(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stle.b", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stle_h(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stle.h", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stle_w(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stle.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stle_d(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "stle.d", true);

    // sc.w, sc.d
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_sc_w(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "sc.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_sc_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "sc.d", true);

    // amswap, amadd, etc.
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amswap_w(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amswap.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amswap_d(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amswap.d", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amadd_w(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amadd.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amadd_d(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amadd.d", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amand_w(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amand.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amor_w(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amor.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amxor_w(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amxor.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amcas_w(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amcas.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_amcas_d(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "amcas.d", true);

    // stl.w, str.w, stl.d, str.d
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stl_w(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "stl.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_str_w(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "str.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_stl_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "stl.d", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_str_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "str.d", true);

    // sc.q
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_sc_q(&asm_, LA_A0, LA_A2, LA_A1);
    failures += test_store(&asm_, "sc.q", true);

    // fstgt, fstle
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fstgt_s(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fstgt.s", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fstgt_d(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fstgt.d", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fstle_s(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fstle.s", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fstle_d(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fstle.d", true);

    // vstelm.*
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vstelm_b(&asm_, LA_V0, LA_A1, 0, 0);
    failures += test_store(&asm_, "vstelm.b", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vstelm_h(&asm_, LA_V0, LA_A1, 0, 0);
    failures += test_store(&asm_, "vstelm.h", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vstelm_w(&asm_, LA_V0, LA_A1, 0, 0);
    failures += test_store(&asm_, "vstelm.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vstelm_d(&asm_, LA_V0, LA_A1, 0, 0);
    failures += test_store(&asm_, "vstelm.d", true);

    // xvstelm.*
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvstelm_b(&asm_, LA_XV0, LA_A1, 0, 0);
    failures += test_store(&asm_, "xvstelm.b", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvstelm_h(&asm_, LA_XV0, LA_A1, 0, 0);
    failures += test_store(&asm_, "xvstelm.h", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvstelm_w(&asm_, LA_XV0, LA_A1, 0, 0);
    failures += test_store(&asm_, "xvstelm.w", true);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvstelm_d(&asm_, LA_XV0, LA_A1, 0, 0);
    failures += test_store(&asm_, "xvstelm.d", true);

    // ====== NEGATIVE TESTS: Non-store instructions ======

    // Load instructions
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ld_b(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "ld.b", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ld_h(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "ld.h", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ld_w(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "ld.w", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ld_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "ld.d", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ldx_b(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "ldx.b", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ldx_w(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "ldx.w", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ldx_d(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "ldx.d", false);

    // Floating-point loads
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fld_s(&asm_, LA_FA0, LA_A1, 0);
    failures += test_store(&asm_, "fld.s", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fld_d(&asm_, LA_FA0, LA_A1, 0);
    failures += test_store(&asm_, "fld.d", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fldx_s(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fldx.s", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fldx_d(&asm_, LA_FA0, LA_A1, LA_A2);
    failures += test_store(&asm_, "fldx.d", false);

    // Vector loads
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vld(&asm_, LA_V0, LA_A1, 0);
    failures += test_store(&asm_, "vld", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_vldx(&asm_, LA_V0, LA_A1, LA_A2);
    failures += test_store(&asm_, "vldx", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvld(&asm_, LA_XV0, LA_A1, 0);
    failures += test_store(&asm_, "xvld", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xvldx(&asm_, LA_XV0, LA_A1, LA_A2);
    failures += test_store(&asm_, "xvldx", false);

    // ll.w, ll.d (not stores)
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ll_w(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "ll.w", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_ll_d(&asm_, LA_A0, LA_A1, 0);
    failures += test_store(&asm_, "ll.d", false);

    // Arithmetic instructions
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_add_w(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "add.w", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_add_d(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "add.d", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_sub_w(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "sub.w", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_and(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "and", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_or(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "or", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_xor(&asm_, LA_A0, LA_A1, LA_A2);
    failures += test_store(&asm_, "xor", false);

    // Branch instructions
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_beq(&asm_, LA_A0, LA_A1, 4);
    failures += test_store(&asm_, "beq", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_bne(&asm_, LA_A0, LA_A1, 4);
    failures += test_store(&asm_, "bne", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_b(&asm_, 4);
    failures += test_store(&asm_, "b", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_bl(&asm_, 4);
    failures += test_store(&asm_, "bl", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_jirl(&asm_, LA_RA, LA_A0, 0);
    failures += test_store(&asm_, "jirl", false);

    // Floating-point operations
    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fadd_s(&asm_, LA_FA0, LA_FA1, LA_FA2);
    failures += test_store(&asm_, "fadd.s", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fadd_d(&asm_, LA_FA0, LA_FA1, LA_FA2);
    failures += test_store(&asm_, "fadd.d", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fmul_s(&asm_, LA_FA0, LA_FA1, LA_FA2);
    failures += test_store(&asm_, "fmul.s", false);

    la_init_assembler(&asm_, buffer, sizeof(buffer));
    la_fdiv_s(&asm_, LA_FA0, LA_FA1, LA_FA2);
    failures += test_store(&asm_, "fdiv.s", false);

    if (failures == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed.\n", failures);
        return 1;
    }
}
