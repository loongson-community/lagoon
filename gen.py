#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import re
import subprocess

SLOT_SHIFTS: dict[str, int] = {"d": 0, "j": 5, "k": 10, "a": 15, "m": 16, "n": 18}

REGISTER_SIZES: dict[str, int] = {"c": 3, "t": 2}  # default 5

TEST_ARG_VALUES: dict[str, list[str]] = {
    "D": ["LA_A0"],
    "J": ["LA_A1"],
    "K": ["LA_A2"],
    "F": ["LA_FA0", "LA_FA1", "LA_FA2", "LA_FA3"],
    "C": ["LA_FCC0", "LA_FCC1", "LA_FCC2", "LA_FCC3"],
    "T": ["LA_FCC0", "LA_FCC1", "LA_FCC2", "LA_FCC3"],
    "V": ["LA_V0", "LA_V1", "LA_V2", "LA_V3"],
    "X": ["LA_XV0", "LA_XV1", "LA_XV2", "LA_XV3"],
    "R": ["LA_FCSR0", "LA_FCSR1", "LA_FCSR2", "LA_FCSR3"],
}

EXPECTED_OUTPUT_VALUES: dict[str, list[str]] = {
    "D": ["$a0"],
    "J": ["$a1"],
    "K": ["$a2"],
    "F": ["$fa0", "$fa1", "$fa2", "$fa3"],
    "C": ["$fcc0", "$fcc1", "$fcc2", "$fcc3"],
    "T": ["$scr0", "$scr1", "$scr2", "$scr3"],
    "V": ["$vr0", "$vr1", "$vr2", "$vr3"],
    "X": ["$xr0", "$xr1", "$xr2", "$xr3"],
    "R": ["$fcsr0", "$fcsr1", "$fcsr2", "$fcsr3"],
}

def parse_slot(slot: str) -> tuple[str, str]:
    lower = slot.lower()
    if "p" in lower:
        base, post = lower.split("p", 1)
        return base, post
    return lower, ""


def register_bit_size(prefix: str) -> int:
    return REGISTER_SIZES.get(prefix, 5)


class Insn:
    encoding: int
    orig_mnemonic: str
    mnemonic: str
    slots: list[str]
    attributes: dict[str, str | bool]

    def __init__(self, line: str) -> None:
        regex = "^([0-9a-f]{8}) ([a-z][0-9a-z_.]*) +(EMPTY|[0-9DJKACFVXSTUdjkamn]+)((?: *@[0-9A-Za-z_.=]+)*)$"
        parts = re.match(regex, line).groups()
        if len(parts) < 3:
            raise ValueError(f"Invalid instruction line: {line}")
        self.encoding = int(parts[0], 16)
        self.orig_mnemonic = parts[1]
        self.mnemonic = self.orig_mnemonic.replace(".", "_")
        self.slots = (
            [] if parts[2] == "EMPTY" else re.findall(r"[A-Z][a-z0-9]*", parts[2])
        )
        raw = re.findall(r"@([0-9A-Za-z_.=]+)", parts[3])
        self.attributes = {}
        for attr in raw:
            key_value = attr.split("=", 1)
            if len(key_value) == 2:
                key, value = key_value
            else:
                key = key_value[0]
                value = True
            self.attributes[key] = value

        if "orig_fmt" in self.attributes:
            self.slots = re.findall(r"[A-Z][a-z0-9]*", self.attributes["orig_fmt"])
        if "orig_name" in self.attributes:
            self.mnemonic = self.attributes["orig_name"].replace(".", "_")
            self.orig_mnemonic = self.attributes["orig_name"]


def read_files(file_paths: list[str]) -> list[str]:
    lines: list[str] = []
    for path in file_paths:
        with open(path, "r", encoding="utf-8") as f:
            lines.extend(f.readlines())
    lines = [line.rstrip("\n") for line in lines]
    return lines


def slot_name(slot: str) -> str:
    return "r" + slot.lower() if len(slot) == 1 else slot.lower()


def slot_type(slot: str, mnemonic: str = "") -> str:
    # Special cases for FCSR instructions
    if mnemonic == "movgr2fcsr" and slot[0] == "D":
        return "la_fcsr_t"
    if mnemonic == "movfcsr2gr" and slot[0] == "J":
        return "la_fcsr_t"
    return {
        "T": "la_scr_t",
        "C": "la_fcc_t",
        "F": "la_fpr_t",
        "V": "la_vpr_t",
        "X": "la_xvpr_t",
        "S": "int32_t",
        "U": "uint32_t",
    }.get(slot[0], "la_gpr_t")


def _parse_imm_items(slot_base: str) -> list[tuple[str, int]]:
    items = re.findall(r"([djkamn])([0-9]+)", slot_base[1:])
    return [(item, int(size_str)) for item, size_str in items]


def _parse_reg_slot(slot_base: str) -> tuple[str, int]:
    s = slot_base
    size = 5
    if s[0] in "cftvx":
        size = register_bit_size(s[0])
        s = s[1:]
    assert len(s) == 1
    return s[0], size


def slot_action_C_code(slot: str, name: str) -> str:
    slot_base, _ = parse_slot(slot)
    items: list[tuple[str, int]] = []
    if slot_base[0] in "su":
        items = _parse_imm_items(slot_base)
    else:
        pos_char, size = _parse_reg_slot(slot_base)
        items = [(pos_char, size)]
    assert len(items) <= 2, f"Too many slot items: {slot_base}"

    results: list[str] = []
    for idx, (item, size_in_bits) in enumerate(items):
        mask = (1 << size_in_bits) - 1
        body = name
        if len(items) == 2 and idx == 0:
            body = f"({name} >> {items[1][1]})"
        assert item in SLOT_SHIFTS, f"Unknown slot item: {item}"
        results.append(f"(({body} & 0x{mask:x}) << {SLOT_SHIFTS[item]})")
    return " | ".join(results)


def slot_bits(slot: str) -> int:
    slot_base, _ = parse_slot(slot)
    if slot_base[0] in "su":
        items = _parse_imm_items(slot_base)
        result = 0
        for item, size in items:
            mask = (1 << size) - 1
            result |= mask << SLOT_SHIFTS[item]
        return result
    else:
        pos_char, size = _parse_reg_slot(slot_base)
        mask = (1 << size) - 1
        return mask << SLOT_SHIFTS[pos_char]


def slot_total_bits(slot: str) -> int:
    """Calculate total bit width of an immediate slot."""
    slot_base, _ = parse_slot(slot)
    if slot_base[0] in "su":
        items = _parse_imm_items(slot_base)
        return sum(size for _, size in items)
    return 0


def slot_assertion_code(slot: str, name: str) -> str:
    """Generate range assertion code for an immediate slot."""
    slot_upper = slot[0] if slot else ""
    if slot_upper not in "SU":
        return ""

    total_bits = slot_total_bits(slot)
    if total_bits == 0:
        return ""

    if slot_upper == "S":
        return f"    LA_ASSERT_SIGNED_RANGE({name}, {total_bits});\n"
    else:
        return f"    LA_ASSERT_UNSIGNED_RANGE({name}, {total_bits});\n"


def slot_decode_C_code(slot: str, insn_var: str, operand_var: str, mnemonic: str = "") -> str:
    orig_slot = slot
    slot_base, postproc = parse_slot(slot)

    # Special cases for FCSR instructions
    if mnemonic == "movgr2fcsr" and orig_slot[0] == "D":
        kind = "LA_OP_FCSR"
        field = "fcsr"
    elif mnemonic == "movfcsr2gr" and orig_slot[0] == "J":
        kind = "LA_OP_FCSR"
        field = "fcsr"
    else:
        kind_map: dict[str, str] = {
            "T": "LA_OP_SCR",
            "C": "LA_OP_FCC",
            "F": "LA_OP_FPR",
            "V": "LA_OP_VPR",
            "X": "LA_OP_XVPR",
            "S": "LA_OP_SIMM",
            "U": "LA_OP_UIMM",
        }
        kind = kind_map.get(orig_slot[0], "LA_OP_GPR")

        field_map: dict[str, str] = {
            "LA_OP_GPR": "gpr",
            "LA_OP_FPR": "fpr",
            "LA_OP_VPR": "vpr",
            "LA_OP_XVPR": "xvpr",
            "LA_OP_FCC": "fcc",
            "LA_OP_SCR": "scr",
            "LA_OP_FCSR": "fcsr",
            "LA_OP_SIMM": "simm",
            "LA_OP_UIMM": "uimm",
        }
        field = field_map[kind]

    if slot_base[0] in "su":
        items = _parse_imm_items(slot_base)
        if len(items) == 2:
            item0, size0 = items[0]
            item1, size1 = items[1]
            mask0 = (1 << size0) - 1
            mask1 = (1 << size1) - 1
            sh0 = SLOT_SHIFTS[item0]
            sh1 = SLOT_SHIFTS[item1]
            extract = f"(int32_t)(((({insn_var} >> {sh0}) & 0x{mask0:x}) << {size1}) | (({insn_var} >> {sh1}) & 0x{mask1:x}))"
            total_bits = size0 + size1
            if slot_base[0] == "s":
                extract = f"(int32_t){extract} << {32 - total_bits} >> {32 - total_bits}"
        else:
            item0, size0 = items[0]
            mask0 = (1 << size0) - 1
            sh0 = SLOT_SHIFTS[item0]
            extract = f"(({insn_var} >> {sh0}) & 0x{mask0:x})"
            if slot_base[0] == "s":
                extract = (
                    f"(int32_t){extract} << {32 - size0} >> {32 - size0}"
                )
    else:
        pos_char, size = _parse_reg_slot(slot_base)
        mask = (1 << size) - 1
        sh = SLOT_SHIFTS[pos_char]
        extract = f"(({insn_var} >> {sh}) & 0x{mask:x})"

    postproc_code = ""
    if postproc:
        if postproc[0] == "p":
            postproc_code = f" + {int(postproc[1:])}"
        elif postproc[0] == "s":
            postproc_code = f" << {int(postproc[1:])}"

    value = f"({extract}{postproc_code})"

    lines: list[str] = []
    lines.append(f"        {operand_var}.kind = {kind};")
    lines.append(f"        {operand_var}.{field} = {value};")
    return "\n".join(lines)


def _lookup_slot_value(table: dict[str, list[str]], slot: str, position: int, mnemonic: str = "") -> str:
    _, postproc = parse_slot(slot)
    slot_char = slot[0].upper()
    if slot_char in "US":
        if postproc and postproc[0] == "p":
            return f"{1 + int(postproc[1:])}"
        elif postproc and postproc[0] == "s":
            return f"{1 << int(postproc[1:])}"
        return "1"
    # Special cases for FCSR instructions
    if mnemonic == "movgr2fcsr" and slot_char == "D":
        slot_char = "R"
    elif mnemonic == "movfcsr2gr" and slot_char == "J":
        slot_char = "R"
    assert slot_char in table, f"Unknown slot type: {slot}"
    values = table[slot_char]
    return values[min(position, len(values) - 1)]


def get_test_argument_for_slot(slot: str, position: int, mnemonic: str = "") -> str:
    return _lookup_slot_value(TEST_ARG_VALUES, slot, position, mnemonic)


def get_expected_output_for_slot(slot: str, position: int, mnemonic: str = "") -> str:
    return _lookup_slot_value(EXPECTED_OUTPUT_VALUES, slot, position, mnemonic)


def map_orig_fmt_to_real_slot_positions(insn: Insn, orig_fmt_slots: list[str]) -> list[int]:
    slot_type_to_position: dict[str, list[int]] = {}
    for i, slot in enumerate(insn.slots):
        st = slot[0].upper()
        if st not in slot_type_to_position:
            slot_type_to_position[st] = []
        slot_type_to_position[st].append(i)

    result: list[int] = []
    for fmt_slot in orig_fmt_slots:
        fmt_slot_type = fmt_slot[0].upper()
        if (
            fmt_slot_type in slot_type_to_position
            and slot_type_to_position[fmt_slot_type]
        ):
            pos = slot_type_to_position[fmt_slot_type].pop(0)
            result.append(pos)
        else:
            result.append(0)

    return result


def replace_anchor(filepath: str, anchor_name: str, content: str, separator: str = "\n\n") -> None:
    with open(filepath, "r", encoding="utf-8") as f:
        text = f.read()
    start_marker = f"//// ANCHOR: {anchor_name} start"
    end_marker = f"//// ANCHOR: {anchor_name} end"
    start = text.index(start_marker)
    end = text.index(end_marker)
    new_text = text[:start] + start_marker + "\n\n" + content + separator + text[end:]
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_text)

def get_declaration_comments() -> dict[str, str]:
    intrinsics_json = "unofficial-loongarch-intrinsics-guide/intrinsics.json"
    if not os.path.exists(intrinsics_json):
        subprocess.run(
            ["python3", "unofficial-loongarch-intrinsics-guide/main.py"],
            check=True,
        )

    with open(intrinsics_json, "r", encoding="utf-8") as f:
        intrinsics = json.load(f)

    comments: dict[str, str] = {}
    for entry in intrinsics:
        if not entry.get("instructions"):
            continue
        instr_name = entry["instructions"][0].replace(".", "_")
        content = entry.get("content", "")
        c_intrinsic = entry.get("c_intrinsic", "")

        desc_match = re.search(
            r"### Description\s*\n+(.*?)(?=\n###|\Z)", content, re.DOTALL
        )
        description = desc_match.group(1).strip() if desc_match else ""

        op_match = re.search(
            r"### Operation\s*\n+```c\+\+\n(.*?)```", content, re.DOTALL
        )
        operation = op_match.group(1).strip() if op_match else ""

        comment_lines = ["\n/**"]
        if description:
            if c_intrinsic:
                comment_lines.append(f" * `{c_intrinsic}`")
                comment_lines.append(" *")
        
            for line in description.split("\n"):
                line = line.strip()
                if line:
                    comment_lines.append(f" * {line}")

            if operation and False: # exclude pseudo code for now
                comment_lines.append(" *")
                comment_lines.append(" * Operation:")
                comment_lines.append(" * ```c")
                for line in operation.split("\n"):
                    comment_lines.append(f" * {line}")
                comment_lines.append(" * ```")
            comment_lines.append(" */")
            comments[instr_name] = "\n".join(comment_lines)

    return comments

def generate_declarations(insns: list[Insn]) -> str:
    comments = get_declaration_comments()
    lines: list[str] = []
    for insn in insns:
        func_name = f"la_{insn.mnemonic}"
        params = ", ".join(
            ["lagoon_assembler_t* assembler"]
            + [f"{slot_type(slot, insn.mnemonic)} {slot_name(slot)}" for slot in insn.slots]
        )
        if insn.mnemonic in comments:
            lines.append(comments[insn.mnemonic])
        lines.append(f"void {func_name}({params});")
    return "\n".join(lines)


def generate_definitions(insns: list[Insn]) -> str:
    functions: list[str] = []
    for insn in insns:
        func_name = f"la_{insn.mnemonic}"
        params = ", ".join(
            ["lagoon_assembler_t* assembler"]
            + [f"{slot_type(slot, insn.mnemonic)} {slot_name(slot)}" for slot in insn.slots]
        )
        raw = " | ".join(
            [f"0x{insn.encoding:08x}"]
            + [slot_action_C_code(slot, slot_name(slot)) for slot in insn.slots]
        )
        postproc_code = ""
        assertion_code = ""
        for slot in insn.slots:
            _, postproc = parse_slot(slot)
            if postproc:
                if postproc[0] == "p":
                    postproc_code += f"    {slot_name(slot)} -= {int(postproc[1:])};\n"
                elif postproc[0] == "s":
                    postproc_code += f"    {slot_name(slot)} >>= {int(postproc[1:])};\n"
                else:
                    assert False, f"Unknown postproc: {postproc}"
            assertion_code += slot_assertion_code(slot, slot_name(slot))

        functions.append(
            f"""\
void {func_name}({params})
{{
{postproc_code}{assertion_code}    emit32(assembler, {raw});
}}"""
        )
    return "\n\n".join(functions)


def generate_disasm_cases(insns: list[Insn]) -> str:
    cases: list[str] = []
    for insn in insns:
        var_bits = 0
        for slot in insn.slots:
            var_bits |= slot_bits(slot)
        fixed_mask = 0xFFFFFFFF & ~var_bits
        operand_lines: list[str] = []
        for i, slot in enumerate(insn.slots):
            operand_lines.append(
                slot_decode_C_code(slot, "word", f"insn->operands[{i}]", insn.mnemonic)
            )
        operand_count = len(insn.slots)
        operands_code = (
            ("\n" + "\n".join(operand_lines) + "\n") if operand_lines else "\n"
        )
        cases.append(
            f"    if ((word & 0x{fixed_mask:08x}u) == 0x{insn.encoding:08x}u) {{\n"
            f'        insn->mnemonic = "{insn.orig_mnemonic}";\n'
            f"        insn->operand_count = {operand_count};{operands_code}"
            f"        return;\n"
            f"    }}"
        )
    return "\n".join(cases)


def generate_smoke_tests(insns: list[Insn]) -> str:
    smoke_tests: list[str] = []
    for insn in insns:
        func_name = f"la_{insn.mnemonic}"

        args: list[str] = []
        for i, slot in enumerate(insn.slots):
            args.append(get_test_argument_for_slot(slot, i, insn.mnemonic))

        if insn.slots:
            if "orig_fmt" in insn.attributes:
                orig_fmt_slots = re.findall(
                    r"[A-Z][a-z0-9]*", insn.attributes["orig_fmt"]
                )
                orig_fmt_positions = map_orig_fmt_to_real_slot_positions(
                    insn, orig_fmt_slots
                )

                expected_parts: list[str] = []
                for pos_in_orig_fmt, original_slot_pos in enumerate(orig_fmt_positions):
                    expected_parts.append(
                        get_expected_output_for_slot(insn.slots[original_slot_pos], original_slot_pos, insn.mnemonic)
                    )
            else:
                expected_parts = []
                for pos in range(len(insn.slots)):
                    expected_parts.append(
                        get_expected_output_for_slot(insn.slots[pos], pos, insn.mnemonic)
                    )

            expected_disasm = (
                '"' + f"{insn.orig_mnemonic} " + ", ".join(expected_parts) + '"'
            )
        else:
            expected_disasm = f'"{insn.orig_mnemonic}"'

        if args:
            test_call = f"    {func_name}(&assembler, {', '.join(args)});"
        else:
            test_call = f"    {func_name}(&assembler);"
        test_check = (
            f"    result += test_disasmbler(*(uint32_t*)buffer, {expected_disasm});"
        )
        reset_buffer = "    la_init_assembler(&assembler, buffer, sizeof(buffer));"

        smoke_tests.append(test_call)
        smoke_tests.append(test_check)
        smoke_tests.append(reset_buffer)
        smoke_tests.append("")

    return "\n".join(smoke_tests)


def main() -> None:
    input_dir = "loongarch-opcodes"
    input_files = [
        os.path.join(input_dir, f) for f in os.listdir(input_dir) if f.endswith(".txt")
    ]
    lines = read_files(input_files)
    insns = sorted(
        [Insn(line) for line in lines if line.strip()], key=lambda insn: insn.mnemonic
    )

    replace_anchor("include/lagoon.h", "mnemonic function declarations", generate_declarations(insns))
    print("Updated include/lagoon.h with generated function declarations.")

    replace_anchor("lagoon.c", "mnemonic function definitions", generate_definitions(insns))
    print("Updated lagoon.c with generated function definitions.")

    replace_anchor("lagoon.c", "disasm", generate_disasm_cases(insns), separator="\n\n    ")
    print("Updated lagoon.c with generated disasm cases.")

    replace_anchor("tests/test_smoke.c", "smoke", generate_smoke_tests(insns), separator="")
    print("Updated tests/test_smoke.c with generated smoke tests.")


if __name__ == "__main__":
    main()
