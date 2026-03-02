#define _GNU_SOURCE
#include "lagoon.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char* dasm(uint32_t insn)
{
    char tmppath[] = "/tmp/lagoon_test_dasm_XXXXXX";
    int fd = mkstemp(tmppath);
    if (fd < 0)
        return NULL;

    uint8_t bytes[4] = {
        (uint8_t)(insn & 0xFF),
        (uint8_t)((insn >> 8) & 0xFF),
        (uint8_t)((insn >> 16) & 0xFF),
        (uint8_t)((insn >> 24) & 0xFF),
    };

    if (write(fd, bytes, 4) != 4) {
        close(fd);
        remove(tmppath);
        return NULL;
    }
    close(fd);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "objdump -b binary -m Loongarch64 -D %s", tmppath);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        remove(tmppath);
        return NULL;
    }

    char* result = NULL;
    char line[1024];
    while (fgets(line, sizeof(line), pipe)) {
        if (!strstr(line, "   0:"))
            continue;

        char buf[1024];
        size_t pos = 0;
        char *tok, *tok_next;
        int field = 0;
        int first = 1;

        tok = strtok(line, " ,\t\n");
        while (tok) {
            tok_next = strtok(NULL, " ,\t\n");
            field++;
            if (tok_next && !strcmp(tok_next, "#"))
                tok_next = NULL;
            if (field >= 3) {
                if (first && !strcmp(tok, ".word")) {
                    memcpy(buf + pos, "unknown", strlen("unknown"));
                    pos += strlen("unknown");
                    break;
                }
                if (strncmp(tok, "0x", 2) == 0 || strncmp(tok, "-0x", 3) == 0) {
                    long val = strtol(tok, NULL, 0);
                    pos += sprintf(buf + pos, "%ld", val);
                } else {
                    size_t tlen = strlen(tok);
                    memcpy(buf + pos, tok, tlen);
                    pos += tlen;
                }

                if (tok_next) {
                    if (!first)
                        buf[pos++] = ',';
                    buf[pos++] = ' ';
                }
                first = 0;
            }
            tok = tok_next;
        }
        buf[pos] = '\0';
        result = strdup(buf);
        break;
    }

    pclose(pipe);

    remove(tmppath);

    return result;
}

static int test_disasmbler(uint32_t instruction, const char* expected_output)
{
    lagoon_insn_t disasm_insn;
    la_disasm_one(instruction, &disasm_insn);

    char disasm_buf[256];
    la_insn_to_str(&disasm_insn, disasm_buf, sizeof(disasm_buf));

    if (strcmp(disasm_buf, expected_output)) {
        printf("Expected: '%s'\n", expected_output);
        printf("Got:      '%s'\n", disasm_buf);
        exit(1);
        return 1;
    }

    char* objdump_output = dasm(instruction);
    if (!objdump_output || !strcmp(objdump_output, "unknown")) {
        if (objdump_output)
            free(objdump_output);
        return 0;
    }
    if (strcmp(objdump_output, expected_output)) {
        printf("Expected(dasm): '%s'\n", objdump_output);
        printf("Got:            '%s'\n", expected_output);
        free(objdump_output);
        return 1;
    }
    free(objdump_output);
    return 0;
}

int smoke_test_all_instructions()
{
    int result = 0;

    uint8_t buffer[16];
    lagoon_assembler_t assembler;
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    // clang-format off
    //// ANCHOR: smoke start

    la_adc_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "adc.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_adc_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "adc.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_adc_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "adc.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_adc_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "adc.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_add_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "add.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_add_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "add.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_addi_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "addi.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_addi_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "addi.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_addu12i_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "addu12i.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_addu12i_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "addu12i.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_addu16i_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "addu16i.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_alsl_d(&assembler, LA_A0, LA_A1, LA_A2, 2);
    result += test_disasmbler(*(uint32_t*)buffer, "alsl.d $a0, $a1, $a2, 2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_alsl_w(&assembler, LA_A0, LA_A1, LA_A2, 2);
    result += test_disasmbler(*(uint32_t*)buffer, "alsl.w $a0, $a1, $a2, 2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_alsl_wu(&assembler, LA_A0, LA_A1, LA_A2, 2);
    result += test_disasmbler(*(uint32_t*)buffer, "alsl.wu $a0, $a1, $a2, 2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_b(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd.b $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_db_b(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd_db.b $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_db_h(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd_db.h $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_h(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd.h $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amadd_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amadd.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amand_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amand.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amand_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amand_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amand_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amand_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amand_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amand.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_b(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas.b $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_db_b(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas_db.b $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_db_h(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas_db.h $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_h(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas.h $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amcas_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amcas.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_db_du(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax_db.du $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_db_wu(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax_db.wu $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_du(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax.du $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammax_wu(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammax.wu $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_db_du(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin_db.du $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_db_wu(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin_db.wu $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_du(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin.du $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ammin_wu(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ammin.wu $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amor_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amor.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amor_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amor_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amor_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amor_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amor_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amor.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_b(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap.b $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_db_b(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap_db.b $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_db_h(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap_db.h $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_h(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap.h $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amswap_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amswap.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amxor_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amxor.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amxor_db_d(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amxor_db.d $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amxor_db_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amxor_db.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_amxor_w(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "amxor.w $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_and(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "and $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_andi(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "andi $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_andn(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "andn $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armadc_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armadc.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armadd_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armadd.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armand_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armand.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armmfflag(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armmfflag $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armmov_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armmov.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armmov_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armmov.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armmove(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armmove $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armmtflag(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armmtflag $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armnot_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armnot.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armor_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armor.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armrotr_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armrotr.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armrotri_w(&assembler, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armrotri.w $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armrrx_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armrrx.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsbc_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsbc.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsll_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsll.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armslli_w(&assembler, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armslli.w $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsra_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsra.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsrai_w(&assembler, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsrai.w $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsrl_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsrl.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsrli_w(&assembler, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsrli.w $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armsub_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armsub.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_armxor_w(&assembler, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "armxor.w $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_asrtgt_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "asrtgt.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_asrtle_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "asrtle.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_b(&assembler, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "b 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bceqz(&assembler, LA_FCC0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bceqz $fcc0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bcnez(&assembler, LA_FCC0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bcnez $fcc0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_beq(&assembler, LA_A1, LA_A0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "beq $a1, $a0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_beqz(&assembler, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "beqz $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bge(&assembler, LA_A1, LA_A0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bge $a1, $a0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bgeu(&assembler, LA_A1, LA_A0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bgeu $a1, $a0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bitrev_4b(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "bitrev.4b $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bitrev_8b(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "bitrev.8b $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bitrev_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "bitrev.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bitrev_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "bitrev.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bl(&assembler, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bl 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_blt(&assembler, LA_A1, LA_A0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "blt $a1, $a0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bltu(&assembler, LA_A1, LA_A0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bltu $a1, $a0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bne(&assembler, LA_A1, LA_A0, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bne $a1, $a0, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bnez(&assembler, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "bnez $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_break(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "break 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bstrins_d(&assembler, LA_A0, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "bstrins.d $a0, $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bstrins_w(&assembler, LA_A0, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "bstrins.w $a0, $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bstrpick_d(&assembler, LA_A0, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "bstrpick.d $a0, $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bstrpick_w(&assembler, LA_A0, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "bstrpick.w $a0, $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bytepick_d(&assembler, LA_A0, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "bytepick.d $a0, $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_bytepick_w(&assembler, LA_A0, LA_A1, LA_A2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "bytepick.w $a0, $a1, $a2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_cacop(&assembler, 1, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "cacop 1, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_clo_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "clo.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_clo_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "clo.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_clz_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "clz.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_clz_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "clz.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_cpucfg(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "cpucfg $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crc_w_b_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crc.w.b.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crc_w_d_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crc.w.d.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crc_w_h_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crc.w.h.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crc_w_w_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crc.w.w.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crcc_w_b_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crcc.w.b.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crcc_w_d_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crcc.w.d.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crcc_w_h_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crcc.w.h.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_crcc_w_w_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "crcc.w.w.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_csrxchg(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "csrxchg $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_cto_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "cto.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_cto_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "cto.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ctz_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ctz.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ctz_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ctz.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_dbar(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "dbar 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_dbcl(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "dbcl 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_div_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "div.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_div_du(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "div.du $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_div_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "div.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_div_wu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "div.wu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ertn(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "ertn");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ext_w_b(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ext.w.b $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ext_w_h(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "ext.w.h $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fabs_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fabs.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fabs_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fabs.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fadd_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fadd.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fadd_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fadd.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fclass_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fclass.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fclass_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fclass.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_caf_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.caf.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_caf_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.caf.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_ceq_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.ceq.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_ceq_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.ceq.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cle_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cle.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cle_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cle.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_clt_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.clt.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_clt_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.clt.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cne_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cne.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cne_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cne.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cor_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cor.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cor_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cor.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cueq_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cueq.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cueq_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cueq.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cule_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cule.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cule_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cule.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cult_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cult.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cult_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cult.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cun_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cun.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cun_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cun.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cune_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cune.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_cune_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.cune.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_saf_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.saf.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_saf_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.saf.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_seq_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.seq.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_seq_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.seq.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sle_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sle.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sle_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sle.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_slt_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.slt.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_slt_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.slt.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sne_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sne.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sne_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sne.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sor_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sor.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sor_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sor.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sueq_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sueq.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sueq_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sueq.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sule_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sule.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sule_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sule.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sult_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sult.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sult_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sult.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sun_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sun.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sun_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sun.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sune_d(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sune.d $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcmp_sune_s(&assembler, LA_FCC0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcmp.sune.s $fcc0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcopysign_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcopysign.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcopysign_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcopysign.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcvt_d_ld(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fcvt.d.ld $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcvt_d_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fcvt.d.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcvt_ld_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fcvt.ld.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcvt_s_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fcvt.s.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fcvt_ud_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fcvt.ud.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fdiv_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fdiv.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fdiv_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fdiv.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ffint_d_l(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ffint.d.l $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ffint_d_w(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ffint.d.w $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ffint_s_l(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ffint.s.l $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ffint_s_w(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ffint.s.w $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fld_d(&assembler, LA_FA0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "fld.d $fa0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fld_s(&assembler, LA_FA0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "fld.s $fa0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fldgt_d(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fldgt.d $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fldgt_s(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fldgt.s $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fldle_d(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fldle.d $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fldle_s(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fldle.s $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fldx_d(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fldx.d $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fldx_s(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fldx.s $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_flogb_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "flogb.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_flogb_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "flogb.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmadd_d(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fmadd.d $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmadd_s(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fmadd.s $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmax_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmax.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmax_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmax.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmaxa_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmaxa.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmaxa_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmaxa.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmin_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmin.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmin_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmin.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmina_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmina.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmina_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmina.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmov_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fmov.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmov_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fmov.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmsub_d(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fmsub.d $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmsub_s(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fmsub.s $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmul_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmul.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fmul_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fmul.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fneg_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fneg.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fneg_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fneg.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fnmadd_d(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fnmadd.d $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fnmadd_s(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fnmadd.s $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fnmsub_d(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fnmsub.d $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fnmsub_s(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FA3);
    result += test_disasmbler(*(uint32_t*)buffer, "fnmsub.s $fa0, $fa1, $fa2, $fa3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frecip_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frecip.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frecip_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frecip.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frecipe_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frecipe.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frecipe_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frecipe.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frint_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frint.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frint_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frint.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frsqrt_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frsqrt.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frsqrt_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frsqrt.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frsqrte_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frsqrte.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_frsqrte_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "frsqrte.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fscaleb_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fscaleb.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fscaleb_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fscaleb.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fsel(&assembler, LA_FA0, LA_FA1, LA_FA2, LA_FCC3);
    result += test_disasmbler(*(uint32_t*)buffer, "fsel $fa0, $fa1, $fa2, $fcc3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fsqrt_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fsqrt.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fsqrt_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "fsqrt.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fst_d(&assembler, LA_FA0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "fst.d $fa0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fst_s(&assembler, LA_FA0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "fst.s $fa0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fstgt_d(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fstgt.d $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fstgt_s(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fstgt.s $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fstle_d(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fstle.d $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fstle_s(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fstle.s $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fstx_d(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fstx.d $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fstx_s(&assembler, LA_FA0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "fstx.s $fa0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fsub_d(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fsub.d $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_fsub_s(&assembler, LA_FA0, LA_FA1, LA_FA2);
    result += test_disasmbler(*(uint32_t*)buffer, "fsub.s $fa0, $fa1, $fa2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftint_l_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftint.l.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftint_l_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftint.l.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftint_w_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftint.w.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftint_w_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftint.w.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrm_l_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrm.l.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrm_l_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrm.l.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrm_w_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrm.w.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrm_w_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrm.w.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrne_l_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrne.l.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrne_l_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrne.l.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrne_w_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrne.w.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrne_w_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrne.w.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrp_l_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrp.l.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrp_l_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrp.l.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrp_w_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrp.w.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrp_w_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrp.w.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrz_l_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrz.l.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrz_l_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrz.l.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrz_w_d(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrz.w.d $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ftintrz_w_s(&assembler, LA_FA0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "ftintrz.w.s $fa0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gcsrxchg(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "gcsrxchg $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gtlbclr(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "gtlbclr");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gtlbfill(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "gtlbfill");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gtlbflush(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "gtlbflush");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gtlbrd(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "gtlbrd");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gtlbsrch(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "gtlbsrch");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_gtlbwr(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "gtlbwr");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_hvcl(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "hvcl 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ibar(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ibar 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_idle(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "idle 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_invtlb(&assembler, 1, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "invtlb 1, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrrd_b(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrrd.b $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrrd_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrrd.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrrd_h(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrrd.h $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrrd_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrrd.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrwr_b(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrwr.b $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrwr_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrwr.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrwr_h(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrwr.h $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_iocsrwr_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "iocsrwr.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_jirl(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "jirl $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_jiscr0(&assembler, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "jiscr0 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_jiscr1(&assembler, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "jiscr1 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_b(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.b $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_bu(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.bu $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_h(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.h $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_hu(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.hu $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ld_wu(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ld.wu $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_lddir(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "lddir $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldgt_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldgt.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldgt_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldgt.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldgt_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldgt.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldgt_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldgt.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldl_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ldl.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldl_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ldl.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldle_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldle.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldle_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldle.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldle_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldle.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldle_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldle.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldpte(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ldpte $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldptr_d(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "ldptr.d $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldptr_w(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "ldptr.w $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldr_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ldr.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldr_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ldr.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_bu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.bu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_hu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.hu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ldx_wu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "ldx.wu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ll_d(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "ll.d $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ll_w(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "ll.w $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_llacq_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "llacq.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_llacq_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "llacq.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_lu12i_w(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "lu12i.w $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_lu32i_d(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "lu32i.d $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_lu52i_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "lu52i.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_maskeqz(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "maskeqz $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_masknez(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "masknez $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mod_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mod.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mod_du(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mod.du $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mod_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mod.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mod_wu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mod.wu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movcf2fr(&assembler, LA_FA0, LA_FCC1);
    result += test_disasmbler(*(uint32_t*)buffer, "movcf2fr $fa0, $fcc1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movcf2gr(&assembler, LA_A0, LA_FCC1);
    result += test_disasmbler(*(uint32_t*)buffer, "movcf2gr $a0, $fcc1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movfcsr2gr(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movfcsr2gr $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movfr2cf(&assembler, LA_FCC0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "movfr2cf $fcc0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movfr2gr_d(&assembler, LA_A0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "movfr2gr.d $a0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movfr2gr_s(&assembler, LA_A0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "movfr2gr.s $a0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movfrh2gr_s(&assembler, LA_A0, LA_FA1);
    result += test_disasmbler(*(uint32_t*)buffer, "movfrh2gr.s $a0, $fa1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movgr2cf(&assembler, LA_FCC0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movgr2cf $fcc0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movgr2fcsr(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movgr2fcsr $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movgr2fr_d(&assembler, LA_FA0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movgr2fr.d $fa0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movgr2fr_w(&assembler, LA_FA0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movgr2fr.w $fa0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movgr2frh_w(&assembler, LA_FA0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movgr2frh.w $fa0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movgr2scr(&assembler, LA_FCC0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "movgr2scr $scr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_movscr2gr(&assembler, LA_A0, LA_FCC1);
    result += test_disasmbler(*(uint32_t*)buffer, "movscr2gr $a0, $scr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mul_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mul.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mul_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mul.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mulh_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mulh.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mulh_du(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mulh.du $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mulh_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mulh.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mulh_wu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mulh.wu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mulw_d_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mulw.d.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_mulw_d_wu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "mulw.d.wu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_nor(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "nor $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_or(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "or $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_ori(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "ori $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_orn(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "orn $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_pcaddi(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "pcaddi $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_pcaddu12i(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "pcaddu12i $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_pcaddu18i(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "pcaddu18i $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_pcalau12i(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "pcalau12i $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_preld(&assembler, 1, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "preld 1, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_preldx(&assembler, 1, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "preldx 1, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcr_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rcr.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcr_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rcr.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcr_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rcr.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcr_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rcr.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcri_b(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rcri.b $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcri_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rcri.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcri_h(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rcri.h $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rcri_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rcri.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rdtime_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "rdtime.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rdtimeh_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "rdtimeh.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rdtimel_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "rdtimel.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_revb_2h(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "revb.2h $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_revb_2w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "revb.2w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_revb_4h(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "revb.4h $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_revb_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "revb.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_revh_2w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "revh.2w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_revh_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "revh.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotr_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rotr.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotr_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rotr.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotr_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rotr.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotr_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "rotr.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotri_b(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rotri.b $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotri_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rotri.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotri_h(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rotri.h $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_rotri_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "rotri.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sbc_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sbc.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sbc_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sbc.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sbc_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sbc.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sbc_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sbc.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sc_d(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "sc.d $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sc_q(&assembler, LA_A0, LA_A2, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "sc.q $a0, $a2, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sc_w(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "sc.w $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_screl_d(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "screl.d $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_screl_w(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "screl.w $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_setarmj(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "setarmj $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_setx86j(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "setx86j $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_setx86loope(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "setx86loope $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_setx86loopne(&assembler, LA_A0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "setx86loopne $a0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sll_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sll.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sll_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sll.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_slli_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "slli.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_slli_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "slli.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_slt(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "slt $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_slti(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "slti $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sltu(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sltu $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sltui(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "sltui $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sra_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sra.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sra_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sra.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_srai_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "srai.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_srai_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "srai.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_srl_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "srl.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_srl_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "srl.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_srli_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "srli.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_srli_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "srli.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_st_b(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "st.b $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_st_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "st.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_st_h(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "st.h $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_st_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "st.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stgt_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stgt.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stgt_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stgt.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stgt_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stgt.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stgt_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stgt.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stl_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "stl.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stl_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "stl.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stle_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stle.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stle_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stle.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stle_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stle.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stle_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stle.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stptr_d(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "stptr.d $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stptr_w(&assembler, LA_A0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "stptr.w $a0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_str_d(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "str.d $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_str_w(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "str.w $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stx_b(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stx.b $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stx_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stx.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stx_h(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stx.h $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_stx_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "stx.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sub_d(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sub.d $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_sub_w(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "sub.w $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_syscall(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "syscall 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_tlbclr(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "tlbclr");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_tlbfill(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "tlbfill");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_tlbflush(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "tlbflush");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_tlbrd(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "tlbrd");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_tlbsrch(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "tlbsrch");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_tlbwr(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "tlbwr");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vabsd_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vabsd.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadd_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadd.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadd_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadd.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadd_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadd.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadd_q(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadd.q $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadd_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadd.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadda_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadda.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadda_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadda.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadda_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadda.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vadda_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vadda.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddi_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddi.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddi_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddi.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddi_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddi.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddi_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddi.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_d_wu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.d.wu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_h_bu_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.h.bu.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_q_du_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.q.du.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwev_w_hu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwev.w.hu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_d_wu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.d.wu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_h_bu_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.h.bu.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_q_du_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.q.du.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vaddwod_w_hu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vaddwod.w.hu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vand_v(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vand.v $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vandi_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vandi.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vandn_v(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vandn.v $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavg_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavg.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vavgr_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vavgr.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclr_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclr.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclr_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclr.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclr_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclr.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclr_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclr.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclri_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclri.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclri_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclri.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclri_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclri.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitclri_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitclri.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrev_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrev.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrev_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrev.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrev_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrev.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrev_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrev.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrevi_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrevi.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrevi_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrevi.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrevi_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrevi.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitrevi_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitrevi.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitsel_v(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitsel.v $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitseli_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitseli.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitset_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitset.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitset_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitset.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitset_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitset.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitset_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitset.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitseti_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitseti.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitseti_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitseti.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitseti_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitseti.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbitseti_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbitseti.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbsll_v(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbsll.v $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vbsrl_v(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vbsrl.v $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclo_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclo.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclo_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclo.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclo_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclo.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclo_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclo.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclz_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclz.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclz_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclz.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclz_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclz.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vclz_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vclz.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vdiv_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vdiv.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_d_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.d.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_d_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.d.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_d_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.d.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_du_bu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.du.bu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_du_hu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.du.hu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_du_wu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.du.wu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_h_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.h.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_hu_bu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.hu.bu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_w_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.w.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_w_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.w.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_wu_bu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.wu.bu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vext2xv_wu_hu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "vext2xv.wu.hu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_d_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.d.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_du_wu(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.du.wu $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_h_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.h.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_hu_bu(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.hu.bu $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_q_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.q.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_qu_du(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.qu.du $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_w_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.w.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vexth_wu_hu(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vexth.wu.hu $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vextl_q_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vextl.q.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vextl_qu_du(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vextl.qu.du $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vextrins_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vextrins.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vextrins_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vextrins.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vextrins_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vextrins.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vextrins_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vextrins.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfadd_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfadd.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfadd_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfadd.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfclass_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfclass.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfclass_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfclass.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_caf_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.caf.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_caf_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.caf.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_ceq_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.ceq.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_ceq_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.ceq.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cle_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cle.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cle_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cle.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_clt_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.clt.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_clt_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.clt.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cne_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cne.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cne_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cne.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cor_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cor.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cor_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cor.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cueq_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cueq.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cueq_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cueq.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cule_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cule.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cule_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cule.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cult_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cult.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cult_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cult.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cun_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cun.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cun_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cun.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cune_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cune.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_cune_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.cune.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_saf_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.saf.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_saf_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.saf.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_seq_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.seq.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_seq_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.seq.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sle_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sle.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sle_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sle.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_slt_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.slt.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_slt_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.slt.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sne_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sne.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sne_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sne.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sor_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sor.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sor_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sor.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sueq_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sueq.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sueq_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sueq.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sule_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sule.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sule_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sule.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sult_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sult.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sult_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sult.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sun_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sun.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sun_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sun.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sune_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sune.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcmp_sune_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcmp.sune.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcvt_h_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcvt.h.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcvt_s_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcvt.s.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcvth_d_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcvth.d.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcvth_s_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcvth.s.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcvtl_d_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcvtl.d.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfcvtl_s_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfcvtl.s.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfdiv_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfdiv.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfdiv_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfdiv.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffint_d_l(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vffint.d.l $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffint_d_lu(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vffint.d.lu $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffint_s_l(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vffint.s.l $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffint_s_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vffint.s.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffint_s_wu(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vffint.s.wu $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffinth_d_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vffinth.d.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vffintl_d_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vffintl.d.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vflogb_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vflogb.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vflogb_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vflogb.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmadd_d(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmadd.d $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmadd_s(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmadd.s $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmax_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmax.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmax_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmax.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmaxa_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmaxa.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmaxa_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmaxa.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmin_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmin.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmin_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmin.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmina_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmina.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmina_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmina.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmsub_d(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmsub.d $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmsub_s(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmsub.s $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmul_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmul.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfmul_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfmul.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfnmadd_d(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfnmadd.d $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfnmadd_s(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfnmadd.s $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfnmsub_d(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfnmsub.d $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfnmsub_s(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vfnmsub.s $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrecip_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrecip.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrecip_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrecip.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrecipe_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrecipe.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrecipe_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrecipe.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrint_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrint.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrint_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrint.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrm_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrm.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrm_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrm.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrne_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrne.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrne_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrne.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrp_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrp.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrp_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrp.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrz_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrz.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrintrz_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrintrz.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrsqrt_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrsqrt.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrsqrt_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrsqrt.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrsqrte_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrsqrte.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrsqrte_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrsqrte.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrstp_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrstp.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrstp_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrstp.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrstpi_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrstpi.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfrstpi_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfrstpi.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfsqrt_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfsqrt.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfsqrt_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vfsqrt.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfsub_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfsub.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vfsub_s(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vfsub.s $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftint_l_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftint.l.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftint_lu_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftint.lu.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftint_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vftint.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftint_w_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftint.w.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftint_wu_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftint.wu.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftinth_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftinth.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintl_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintl.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrm_l_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrm.l.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrm_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrm.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrm_w_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrm.w.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrmh_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrmh.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrml_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrml.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrne_l_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrne.l.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrne_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrne.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrne_w_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrne.w.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrneh_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrneh.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrnel_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrnel.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrp_l_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrp.l.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrp_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrp.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrp_w_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrp.w.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrph_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrph.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrpl_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrpl.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrz_l_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrz.l.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrz_lu_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrz.lu.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrz_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrz.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrz_w_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrz.w.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrz_wu_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrz.wu.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrzh_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrzh.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vftintrzl_l_s(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vftintrzl.l.s $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_du_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.du.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_hu_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.hu.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_qu_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.qu.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhaddw_wu_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhaddw.wu.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_du_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.du.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_hu_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.hu.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_qu_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.qu.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vhsubw_wu_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vhsubw.wu.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvh_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvh.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvh_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvh.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvh_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvh.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvh_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvh.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvl_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvl.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvl_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvl.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvl_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvl.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vilvl_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vilvl.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vinsgr2vr_b(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vinsgr2vr.b $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vinsgr2vr_d(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vinsgr2vr.d $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vinsgr2vr_h(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vinsgr2vr.h $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vinsgr2vr_w(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vinsgr2vr.w $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vld(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vld $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vldi(&assembler, LA_V0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vldi $vr0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vldrepl_b(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vldrepl.b $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vldrepl_d(&assembler, LA_V0, LA_A1, 8);
    result += test_disasmbler(*(uint32_t*)buffer, "vldrepl.d $vr0, $a1, 8");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vldrepl_h(&assembler, LA_V0, LA_A1, 2);
    result += test_disasmbler(*(uint32_t*)buffer, "vldrepl.h $vr0, $a1, 2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vldrepl_w(&assembler, LA_V0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "vldrepl.w $vr0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vldx(&assembler, LA_V0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "vldx $vr0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmadd_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmadd.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmadd_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmadd.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmadd_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmadd.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmadd_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmadd.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_d_wu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.d.wu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_h_bu_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.h.bu.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_q_du_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.q.du.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwev_w_hu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwev.w.hu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_d_wu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.d.wu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_h_bu_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.h.bu.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_q_du_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.q.du.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaddwod_w_hu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaddwod.w.hu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmax_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmax.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmaxi_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmaxi.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmin_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmin.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmini_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmini.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmod_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmod.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmskgez_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmskgez.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmskltz_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmskltz.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmskltz_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmskltz.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmskltz_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmskltz.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmskltz_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmskltz.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmsknz_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vmsknz.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmsub_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmsub.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmsub_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmsub.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmsub_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmsub.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmsub_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmsub.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmuh_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmuh.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmul_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmul.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmul_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmul.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmul_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmul.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmul_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmul.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_d_wu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.d.wu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_h_bu_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.h.bu.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_q_du_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.q.du.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwev_w_hu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwev.w.hu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_d_wu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.d.wu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_h_bu_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.h.bu.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_q_du_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.q.du.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vmulwod_w_hu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vmulwod.w.hu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vneg_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vneg.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vneg_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vneg.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vneg_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vneg.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vneg_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vneg.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vnor_v(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vnor.v $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vnori_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vnori.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vor_v(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vor.v $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vori_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vori.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vorn_v(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vorn.v $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackev_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackev.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackev_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackev.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackev_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackev.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackev_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackev.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackod_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackod.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackod_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackod.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackod_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackod.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpackod_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpackod.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpcnt_b(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpcnt.b $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpcnt_d(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpcnt.d $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpcnt_h(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpcnt.h $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpcnt_w(&assembler, LA_V0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpcnt.w $vr0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpermi_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpermi.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickev_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickev.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickev_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickev.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickev_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickev.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickev_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickev.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickod_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickod.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickod_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickod.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickod_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickod.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickod_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickod.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_b(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.b $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_bu(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.bu $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_d(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.d $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_du(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.du $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_h(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.h $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_hu(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.hu $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_w(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.w $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vpickve2gr_wu(&assembler, LA_A0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vpickve2gr.wu $a0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplgr2vr_b(&assembler, LA_V0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplgr2vr.b $vr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplgr2vr_d(&assembler, LA_V0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplgr2vr.d $vr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplgr2vr_h(&assembler, LA_V0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplgr2vr.h $vr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplgr2vr_w(&assembler, LA_V0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplgr2vr.w $vr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplve_b(&assembler, LA_V0, LA_V1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplve.b $vr0, $vr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplve_d(&assembler, LA_V0, LA_V1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplve.d $vr0, $vr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplve_h(&assembler, LA_V0, LA_V1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplve.h $vr0, $vr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplve_w(&assembler, LA_V0, LA_V1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplve.w $vr0, $vr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplvei_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplvei.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplvei_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplvei.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplvei_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplvei.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vreplvei_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vreplvei.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotr_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotr.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotr_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotr.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotr_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotr.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotr_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotr.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotri_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotri.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotri_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotri.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotri_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotri.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vrotri_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vrotri.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsadd_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsadd.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsat_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsat.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseq_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vseq.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseq_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vseq.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseq_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vseq.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseq_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vseq.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseqi_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vseqi.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseqi_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vseqi.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseqi_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vseqi.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseqi_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vseqi.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetallnez_b(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetallnez.b $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetallnez_d(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetallnez.d $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetallnez_h(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetallnez.h $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetallnez_w(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetallnez.w $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetanyeqz_b(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetanyeqz.b $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetanyeqz_d(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetanyeqz.d $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetanyeqz_h(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetanyeqz.h $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetanyeqz_w(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetanyeqz.w $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vseteqz_v(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vseteqz.v $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsetnez_v(&assembler, LA_FCC0, LA_V1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsetnez.v $fcc0, $vr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf4i_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf4i.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf4i_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf4i.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf4i_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf4i.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf4i_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf4i.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf_b(&assembler, LA_V0, LA_V1, LA_V2, LA_V3);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf.b $vr0, $vr1, $vr2, $vr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vshuf_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vshuf.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsigncov_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsigncov.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsigncov_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsigncov.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsigncov_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsigncov.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsigncov_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsigncov.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsle_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsle.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslei_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslei.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsll_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsll.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsll_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsll.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsll_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsll.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsll_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsll.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslli_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslli.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslli_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslli.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslli_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslli.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslli_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslli.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsllwil_d_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsllwil.d.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsllwil_du_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsllwil.du.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsllwil_h_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsllwil.h.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsllwil_hu_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsllwil.hu.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsllwil_w_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsllwil.w.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsllwil_wu_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsllwil.wu.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslt_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vslt.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vslti_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vslti.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsra_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsra.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsra_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsra.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsra_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsra.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsra_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsra.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrai_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrai.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrai_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrai.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrai_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrai.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrai_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrai.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsran_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsran.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsran_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsran.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsran_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsran.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrani_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrani.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrani_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrani.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrani_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrani.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrani_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrani.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrar_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrar.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrar_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrar.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrar_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrar.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrar_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrar.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrari_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrari.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrari_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrari.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrari_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrari.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrari_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrari.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarn_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarn.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarn_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarn.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarn_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarn.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarni_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarni.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarni_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarni.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarni_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarni.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrarni_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrarni.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrl_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrl.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrl_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrl.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrl_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrl.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrl_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrl.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrli_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrli.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrli_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrli.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrli_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrli.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrli_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrli.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrln_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrln.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrln_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrln.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrln_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrln.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlni_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlni.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlni_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlni.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlni_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlni.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlni_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlni.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlr_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlr.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlr_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlr.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlr_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlr.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlr_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlr.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlri_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlri.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlri_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlri.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlri_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlri.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlri_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlri.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrn_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrn.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrn_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrn.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrn_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrn.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrni_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrni.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrni_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrni.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrni_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrni.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsrlrni_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsrlrni.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssran_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssran.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssran_bu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssran.bu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssran_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssran.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssran_hu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssran.hu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssran_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssran.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssran_wu_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssran.wu.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_bu_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.bu.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_du_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.du.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_hu_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.hu.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrani_wu_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrani.wu.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarn_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarn.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarn_bu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarn.bu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarn_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarn.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarn_hu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarn.hu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarn_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarn.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarn_wu_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarn.wu.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_bu_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.bu.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_du_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.du.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_hu_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.hu.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrarni_wu_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrarni.wu.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrln_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrln.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrln_bu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrln.bu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrln_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrln.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrln_hu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrln.hu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrln_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrln.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrln_wu_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrln.wu.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_bu_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.bu.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_du_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.du.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_hu_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.hu.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlni_wu_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlni.wu.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrn_b_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrn.b.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrn_bu_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrn.bu.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrn_h_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrn.h.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrn_hu_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrn.hu.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrn_w_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrn.w.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrn_wu_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrn.wu.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_b_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.b.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_bu_h(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.bu.h $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_d_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.d.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_du_q(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.du.q $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_h_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.h.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_hu_w(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.hu.w $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_w_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.w.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssrlrni_wu_d(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vssrlrni.wu.d $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vssub_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vssub.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vst(&assembler, LA_V0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vst $vr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vstelm_b(&assembler, LA_V0, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vstelm.b $vr0, $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vstelm_d(&assembler, LA_V0, LA_A1, 8, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vstelm.d $vr0, $a1, 8, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vstelm_h(&assembler, LA_V0, LA_A1, 2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vstelm.h $vr0, $a1, 2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vstelm_w(&assembler, LA_V0, LA_A1, 4, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vstelm.w $vr0, $a1, 4, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vstx(&assembler, LA_V0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "vstx $vr0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsub_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsub.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsub_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsub.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsub_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsub.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsub_q(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsub.q $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsub_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsub.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubi_bu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubi.bu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubi_du(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubi.du $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubi_hu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubi.hu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubi_wu(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubi.wu $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwev_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwev.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_d_w(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.d.w $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_d_wu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.d.wu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_h_b(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.h.b $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_h_bu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.h.bu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_q_d(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.q.d $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_q_du(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.q.du $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_w_h(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.w.h $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vsubwod_w_hu(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vsubwod.w.hu $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vxor_v(&assembler, LA_V0, LA_V1, LA_V2);
    result += test_disasmbler(*(uint32_t*)buffer, "vxor.v $vr0, $vr1, $vr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_vxori_b(&assembler, LA_V0, LA_V1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "vxori.b $vr0, $vr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86adc_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86adc.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86adc_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86adc.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86adc_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86adc.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86adc_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86adc.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86add_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86add.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86add_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86add.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86add_du(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86add.du $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86add_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86add.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86add_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86add.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86add_wu(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86add.wu $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86and_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86and.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86and_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86and.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86and_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86and.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86and_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86and.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86clrtm(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "x86clrtm");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86dec_b(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86dec.b $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86dec_d(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86dec.d $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86dec_h(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86dec.h $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86dec_w(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86dec.w $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86dectop(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "x86dectop");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86inc_b(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86inc.b $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86inc_d(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86inc.d $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86inc_h(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86inc.h $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86inc_w(&assembler, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86inc.w $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86inctop(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "x86inctop");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mfflag(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mfflag $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mftop(&assembler, LA_A0);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mftop $a0");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mtflag(&assembler, LA_A0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mtflag $a0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mttop(&assembler, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mttop 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_bu(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.bu $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_du(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.du $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_hu(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.hu $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86mul_wu(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86mul.wu $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86or_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86or.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86or_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86or.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86or_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86or.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86or_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86or.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcl_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcl.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcl_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcl.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcl_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcl.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcl_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcl.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcli_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcli.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcli_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcli.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcli_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcli.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcli_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcli.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcr_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcr.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcr_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcr.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcr_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcr.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcr_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcr.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcri_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcri.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcri_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcri.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcri_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcri.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rcri_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rcri.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotl_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotl.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotl_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotl.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotl_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotl.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotl_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotl.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotli_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotli.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotli_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotli.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotli_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotli.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotli_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotli.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotr_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotr.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotr_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotr.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotr_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotr.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotr_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotr.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotri_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotri.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotri_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotri.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotri_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotri.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86rotri_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86rotri.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sbc_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sbc.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sbc_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sbc.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sbc_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sbc.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sbc_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sbc.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86settag(&assembler, LA_A0, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86settag $a0, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86settm(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "x86settm");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sll_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sll.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sll_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sll.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sll_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sll.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sll_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sll.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86slli_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86slli.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86slli_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86slli.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86slli_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86slli.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86slli_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86slli.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sra_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sra.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sra_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sra.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sra_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sra.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sra_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sra.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srai_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srai.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srai_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srai.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srai_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srai.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srai_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srai.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srl_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srl.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srl_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srl.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srl_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srl.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srl_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srl.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srli_b(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srli.b $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srli_d(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srli.d $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srli_h(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srli.h $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86srli_w(&assembler, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "x86srli.w $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sub_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sub.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sub_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sub.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sub_du(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sub.du $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sub_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sub.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sub_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sub.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86sub_wu(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86sub.wu $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86xor_b(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86xor.b $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86xor_d(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86xor.d $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86xor_h(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86xor.h $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_x86xor_w(&assembler, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "x86xor.w $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xor(&assembler, LA_A0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xor $a0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xori(&assembler, LA_A0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xori $a0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvabsd_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvabsd.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadd_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadd.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadd_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadd.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadd_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadd.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadd_q(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadd.q $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadd_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadd.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadda_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadda.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadda_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadda.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadda_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadda.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvadda_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvadda.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddi_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddi.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddi_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddi.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddi_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddi.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddi_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddi.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_d_wu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.d.wu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_h_bu_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.h.bu.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_q_du_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.q.du.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwev_w_hu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwev.w.hu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_d_wu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.d.wu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_h_bu_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.h.bu.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_q_du_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.q.du.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvaddwod_w_hu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvaddwod.w.hu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvand_v(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvand.v $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvandi_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvandi.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvandn_v(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvandn.v $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavg_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavg.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvavgr_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvavgr.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclr_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclr.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclr_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclr.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclr_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclr.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclr_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclr.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclri_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclri.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclri_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclri.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclri_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclri.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitclri_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitclri.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrev_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrev.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrev_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrev.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrev_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrev.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrev_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrev.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrevi_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrevi.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrevi_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrevi.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrevi_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrevi.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitrevi_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitrevi.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitsel_v(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitsel.v $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitseli_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitseli.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitset_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitset.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitset_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitset.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitset_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitset.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitset_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitset.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitseti_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitseti.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitseti_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitseti.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitseti_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitseti.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbitseti_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbitseti.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbsll_v(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbsll.v $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvbsrl_v(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvbsrl.v $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclo_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclo.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclo_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclo.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclo_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclo.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclo_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclo.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclz_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclz.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclz_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclz.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclz_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclz.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvclz_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvclz.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvdiv_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvdiv.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_d_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.d.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_du_wu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.du.wu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_h_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.h.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_hu_bu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.hu.bu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_q_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.q.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_qu_du(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.qu.du $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_w_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.w.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvexth_wu_hu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvexth.wu.hu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvextl_q_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvextl.q.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvextl_qu_du(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvextl.qu.du $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvextrins_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvextrins.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvextrins_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvextrins.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvextrins_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvextrins.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvextrins_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvextrins.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfadd_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfadd.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfadd_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfadd.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfclass_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfclass.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfclass_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfclass.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_caf_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.caf.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_caf_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.caf.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_ceq_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.ceq.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_ceq_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.ceq.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cle_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cle.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cle_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cle.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_clt_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.clt.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_clt_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.clt.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cne_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cne.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cne_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cne.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cor_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cor.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cor_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cor.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cueq_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cueq.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cueq_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cueq.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cule_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cule.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cule_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cule.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cult_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cult.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cult_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cult.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cun_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cun.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cun_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cun.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cune_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cune.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_cune_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.cune.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_saf_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.saf.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_saf_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.saf.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_seq_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.seq.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_seq_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.seq.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sle_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sle.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sle_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sle.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_slt_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.slt.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_slt_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.slt.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sne_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sne.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sne_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sne.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sor_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sor.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sor_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sor.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sueq_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sueq.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sueq_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sueq.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sule_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sule.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sule_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sule.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sult_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sult.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sult_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sult.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sun_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sun.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sun_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sun.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sune_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sune.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcmp_sune_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcmp.sune.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcvt_h_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcvt.h.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcvt_s_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcvt.s.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcvth_d_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcvth.d.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcvth_s_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcvth.s.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcvtl_d_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcvtl.d.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfcvtl_s_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfcvtl.s.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfdiv_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfdiv.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfdiv_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfdiv.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffint_d_l(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffint.d.l $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffint_d_lu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffint.d.lu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffint_s_l(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffint.s.l $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffint_s_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffint.s.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffint_s_wu(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffint.s.wu $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffinth_d_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffinth.d.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvffintl_d_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvffintl.d.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvflogb_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvflogb.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvflogb_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvflogb.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmadd_d(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmadd.d $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmadd_s(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmadd.s $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmax_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmax.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmax_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmax.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmaxa_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmaxa.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmaxa_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmaxa.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmin_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmin.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmin_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmin.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmina_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmina.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmina_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmina.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmsub_d(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmsub.d $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmsub_s(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmsub.s $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmul_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmul.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfmul_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfmul.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfnmadd_d(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfnmadd.d $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfnmadd_s(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfnmadd.s $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfnmsub_d(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfnmsub.d $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfnmsub_s(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfnmsub.s $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrecip_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrecip.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrecip_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrecip.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrecipe_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrecipe.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrecipe_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrecipe.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrint_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrint.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrint_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrint.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrm_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrm.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrm_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrm.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrne_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrne.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrne_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrne.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrp_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrp.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrp_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrp.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrz_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrz.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrintrz_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrintrz.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrsqrt_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrsqrt.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrsqrt_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrsqrt.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrsqrte_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrsqrte.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrsqrte_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrsqrte.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrstp_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrstp.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrstp_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrstp.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrstpi_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrstpi.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfrstpi_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfrstpi.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfsqrt_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfsqrt.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfsqrt_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfsqrt.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfsub_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfsub.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvfsub_s(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvfsub.s $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftint_l_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftint.l.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftint_lu_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftint.lu.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftint_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftint.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftint_w_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftint.w.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftint_wu_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftint.wu.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftinth_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftinth.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintl_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintl.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrm_l_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrm.l.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrm_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrm.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrm_w_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrm.w.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrmh_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrmh.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrml_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrml.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrne_l_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrne.l.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrne_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrne.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrne_w_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrne.w.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrneh_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrneh.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrnel_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrnel.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrp_l_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrp.l.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrp_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrp.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrp_w_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrp.w.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrph_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrph.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrpl_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrpl.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrz_l_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrz.l.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrz_lu_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrz.lu.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrz_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrz.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrz_w_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrz.w.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrz_wu_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrz.wu.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrzh_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrzh.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvftintrzl_l_s(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvftintrzl.l.s $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_du_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.du.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_hu_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.hu.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_qu_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.qu.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhaddw_wu_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhaddw.wu.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_du_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.du.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_hu_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.hu.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_qu_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.qu.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvhsubw_wu_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvhsubw.wu.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvh_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvh.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvh_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvh.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvh_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvh.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvh_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvh.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvl_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvl.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvl_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvl.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvl_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvl.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvilvl_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvilvl.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvinsgr2vr_d(&assembler, LA_XV0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvinsgr2vr.d $xr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvinsgr2vr_w(&assembler, LA_XV0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvinsgr2vr.w $xr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvinsve0_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvinsve0.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvinsve0_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvinsve0.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvld(&assembler, LA_XV0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvld $xr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvldi(&assembler, LA_XV0, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvldi $xr0, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvldrepl_b(&assembler, LA_XV0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvldrepl.b $xr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvldrepl_d(&assembler, LA_XV0, LA_A1, 8);
    result += test_disasmbler(*(uint32_t*)buffer, "xvldrepl.d $xr0, $a1, 8");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvldrepl_h(&assembler, LA_XV0, LA_A1, 2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvldrepl.h $xr0, $a1, 2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvldrepl_w(&assembler, LA_XV0, LA_A1, 4);
    result += test_disasmbler(*(uint32_t*)buffer, "xvldrepl.w $xr0, $a1, 4");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvldx(&assembler, LA_XV0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvldx $xr0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmadd_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmadd.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmadd_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmadd.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmadd_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmadd.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmadd_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmadd.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_d_wu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.d.wu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_h_bu_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.h.bu.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_q_du_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.q.du.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwev_w_hu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwev.w.hu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_d_wu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.d.wu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_h_bu_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.h.bu.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_q_du_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.q.du.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaddwod_w_hu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaddwod.w.hu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmax_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmax.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmaxi_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmaxi.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmin_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmin.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmini_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmini.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmod_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmod.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmskgez_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmskgez.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmskltz_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmskltz.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmskltz_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmskltz.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmskltz_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmskltz.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmskltz_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmskltz.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmsknz_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmsknz.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmsub_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmsub.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmsub_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmsub.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmsub_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmsub.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmsub_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmsub.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmuh_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmuh.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmul_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmul.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmul_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmul.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmul_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmul.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmul_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmul.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_d_wu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.d.wu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_h_bu_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.h.bu.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_q_du_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.q.du.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwev_w_hu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwev.w.hu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_d_wu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.d.wu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_h_bu_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.h.bu.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_q_du_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.q.du.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvmulwod_w_hu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvmulwod.w.hu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvneg_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvneg.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvneg_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvneg.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvneg_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvneg.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvneg_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvneg.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvnor_v(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvnor.v $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvnori_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvnori.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvor_v(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvor.v $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvori_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvori.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvorn_v(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvorn.v $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackev_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackev.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackev_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackev.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackev_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackev.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackev_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackev.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackod_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackod.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackod_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackod.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackod_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackod.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpackod_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpackod.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpcnt_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpcnt.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpcnt_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpcnt.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpcnt_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpcnt.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpcnt_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpcnt.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvperm_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvperm.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpermi_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpermi.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpermi_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpermi.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpermi_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpermi.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickev_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickev.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickev_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickev.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickev_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickev.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickev_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickev.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickod_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickod.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickod_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickod.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickod_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickod.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickod_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickod.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickve2gr_d(&assembler, LA_A0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickve2gr.d $a0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickve2gr_du(&assembler, LA_A0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickve2gr.du $a0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickve2gr_w(&assembler, LA_A0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickve2gr.w $a0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickve2gr_wu(&assembler, LA_A0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickve2gr.wu $a0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickve_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickve.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvpickve_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvpickve.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrepl128vei_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrepl128vei.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrepl128vei_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrepl128vei.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrepl128vei_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrepl128vei.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrepl128vei_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrepl128vei.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplgr2vr_b(&assembler, LA_XV0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplgr2vr.b $xr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplgr2vr_d(&assembler, LA_XV0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplgr2vr.d $xr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplgr2vr_h(&assembler, LA_XV0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplgr2vr.h $xr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplgr2vr_w(&assembler, LA_XV0, LA_A1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplgr2vr.w $xr0, $a1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve0_b(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve0.b $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve0_d(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve0.d $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve0_h(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve0.h $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve0_q(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve0.q $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve0_w(&assembler, LA_XV0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve0.w $xr0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve_b(&assembler, LA_XV0, LA_XV1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve.b $xr0, $xr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve_d(&assembler, LA_XV0, LA_XV1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve.d $xr0, $xr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve_h(&assembler, LA_XV0, LA_XV1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve.h $xr0, $xr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvreplve_w(&assembler, LA_XV0, LA_XV1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvreplve.w $xr0, $xr1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotr_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotr.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotr_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotr.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotr_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotr.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotr_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotr.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotri_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotri.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotri_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotri.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotri_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotri.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvrotri_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvrotri.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsadd_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsadd.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsat_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsat.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseq_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseq.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseq_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseq.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseq_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseq.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseq_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseq.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseqi_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseqi.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseqi_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseqi.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseqi_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseqi.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseqi_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseqi.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetallnez_b(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetallnez.b $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetallnez_d(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetallnez.d $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetallnez_h(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetallnez.h $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetallnez_w(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetallnez.w $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetanyeqz_b(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetanyeqz.b $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetanyeqz_d(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetanyeqz.d $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetanyeqz_h(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetanyeqz.h $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetanyeqz_w(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetanyeqz.w $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvseteqz_v(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvseteqz.v $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsetnez_v(&assembler, LA_FCC0, LA_XV1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsetnez.v $fcc0, $xr1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf4i_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf4i.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf4i_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf4i.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf4i_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf4i.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf4i_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf4i.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf_b(&assembler, LA_XV0, LA_XV1, LA_XV2, LA_XV3);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf.b $xr0, $xr1, $xr2, $xr3");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvshuf_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvshuf.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsigncov_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsigncov.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsigncov_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsigncov.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsigncov_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsigncov.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsigncov_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsigncov.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsle_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsle.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslei_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslei.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsll_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsll.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsll_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsll.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsll_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsll.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsll_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsll.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslli_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslli.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslli_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslli.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslli_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslli.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslli_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslli.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsllwil_d_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsllwil.d.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsllwil_du_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsllwil.du.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsllwil_h_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsllwil.h.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsllwil_hu_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsllwil.hu.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsllwil_w_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsllwil.w.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsllwil_wu_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsllwil.wu.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslt_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslt.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvslti_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvslti.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsra_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsra.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsra_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsra.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsra_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsra.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsra_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsra.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrai_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrai.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrai_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrai.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrai_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrai.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrai_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrai.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsran_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsran.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsran_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsran.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsran_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsran.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrani_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrani.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrani_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrani.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrani_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrani.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrani_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrani.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrar_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrar.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrar_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrar.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrar_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrar.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrar_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrar.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrari_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrari.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrari_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrari.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrari_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrari.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrari_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrari.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarn_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarn.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarn_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarn.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarn_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarn.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarni_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarni.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarni_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarni.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarni_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarni.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrarni_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrarni.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrl_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrl.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrl_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrl.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrl_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrl.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrl_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrl.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrli_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrli.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrli_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrli.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrli_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrli.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrli_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrli.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrln_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrln.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrln_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrln.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrln_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrln.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlni_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlni.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlni_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlni.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlni_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlni.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlni_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlni.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlr_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlr.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlr_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlr.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlr_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlr.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlr_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlr.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlri_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlri.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlri_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlri.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlri_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlri.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlri_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlri.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrn_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrn.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrn_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrn.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrn_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrn.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrni_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrni.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrni_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrni.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrni_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrni.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsrlrni_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsrlrni.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssran_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssran.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssran_bu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssran.bu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssran_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssran.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssran_hu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssran.hu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssran_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssran.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssran_wu_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssran.wu.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_bu_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.bu.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_du_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.du.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_hu_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.hu.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrani_wu_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrani.wu.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarn_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarn.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarn_bu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarn.bu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarn_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarn.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarn_hu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarn.hu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarn_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarn.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarn_wu_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarn.wu.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_bu_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.bu.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_du_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.du.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_hu_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.hu.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrarni_wu_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrarni.wu.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrln_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrln.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrln_bu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrln.bu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrln_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrln.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrln_hu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrln.hu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrln_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrln.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrln_wu_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrln.wu.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_bu_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.bu.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_du_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.du.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_hu_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.hu.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlni_wu_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlni.wu.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrn_b_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrn.b.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrn_bu_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrn.bu.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrn_h_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrn.h.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrn_hu_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrn.hu.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrn_w_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrn.w.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrn_wu_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrn.wu.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_b_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.b.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_bu_h(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.bu.h $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_d_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.d.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_du_q(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.du.q $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_h_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.h.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_hu_w(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.hu.w $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_w_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.w.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssrlrni_wu_d(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssrlrni.wu.d $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvssub_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvssub.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvst(&assembler, LA_XV0, LA_A1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvst $xr0, $a1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvstelm_b(&assembler, LA_XV0, LA_A1, 1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvstelm.b $xr0, $a1, 1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvstelm_d(&assembler, LA_XV0, LA_A1, 8, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvstelm.d $xr0, $a1, 8, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvstelm_h(&assembler, LA_XV0, LA_A1, 2, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvstelm.h $xr0, $a1, 2, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvstelm_w(&assembler, LA_XV0, LA_A1, 4, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvstelm.w $xr0, $a1, 4, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvstx(&assembler, LA_XV0, LA_A1, LA_A2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvstx $xr0, $a1, $a2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsub_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsub.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsub_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsub.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsub_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsub.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsub_q(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsub.q $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsub_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsub.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubi_bu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubi.bu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubi_du(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubi.du $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubi_hu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubi.hu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubi_wu(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubi.wu $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwev_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwev.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_d_w(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.d.w $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_d_wu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.d.wu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_h_b(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.h.b $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_h_bu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.h.bu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_q_d(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.q.d $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_q_du(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.q.du $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_w_h(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.w.h $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvsubwod_w_hu(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvsubwod.w.hu $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvxor_v(&assembler, LA_XV0, LA_XV1, LA_XV2);
    result += test_disasmbler(*(uint32_t*)buffer, "xvxor.v $xr0, $xr1, $xr2");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xvxori_b(&assembler, LA_XV0, LA_XV1, 1);
    result += test_disasmbler(*(uint32_t*)buffer, "xvxori.b $xr0, $xr1, 1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));

    la_xxx_unknown_1(&assembler);
    result += test_disasmbler(*(uint32_t*)buffer, "xxx.unknown.1");
    la_init_assembler(&assembler, buffer, sizeof(buffer));
//// ANCHOR: smoke end
    // clang-format on

    return result;
}

int main()
{
    int result = 0;
    result += smoke_test_all_instructions();

    if (result == 0) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }

    return result;
}