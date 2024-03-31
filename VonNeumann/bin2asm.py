import numpy as np
import sys

import decoder
import isa

def decode_reg(reg_number: int) -> str:
    assert isa.is_reg(reg_number)
    if reg_number < len(isa.SPECIAL_REGISTERS_TO_NAME):
        return isa.SPECIAL_REGISTERS_TO_NAME[reg_number]
    return f"r{reg_number - len(isa.SPECIAL_REGISTERS_TO_NAME) + 1}"


def decode_one_arg(instruction: str, opcode: int, *args) -> str:
    instruction += " "
    if isa.is_reg_instr(opcode):
        instruction += "*" * args[0] + decode_reg(args[1])
    else:
        assert isa.is_imm_instr(opcode)
        instruction += str(args[0])
    return instruction


def decode_two_args(instruction: str, opcode: int, *args) -> str:
    instruction += " "
    if isa.is_reg_reg_instr(opcode):
        instruction += "*" * args[0] + decode_reg(args[1]) + " "
        instruction += "*" * args[2] + decode_reg(args[3])
    else:
        assert isa.is_reg_imm_instr(opcode)
        instruction += "*" * args[0] + decode_reg(args[1]) + " "
        instruction += str(args[2])
    return instruction


def get_from_string_table(static_data, num):
    offset = static_data[num]
    strlen = static_data[offset]
    str = ""
    for i in range(strlen):
        str += chr(static_data[offset + i + 1])
    return str


def main():
    if len(sys.argv) < 2:
        print("Specify BIN file")

    if len(sys.argv) < 3:
        print("Specify output ASM filename")

    disasm = []

    code = np.fromfile(sys.argv[1], dtype=np.uint32)
    code_length = code[0]

    for i in range(1, code_length + 1):
        instr_decoded = decoder.decode(code[i])
        opcode: int = instr_decoded[0]

        instruction: str = isa.OPCODE_TO_NAME[opcode]

        if opcode == isa.NAME_TO_OPCODE["PRINTSTR"]:
            instruction += " " + get_from_string_table(code[code_length + 1:], instr_decoded[1])
        else:
            num_args = isa.OPCODE_TO_NUM_ARGS[opcode]
            if num_args == 1:
                instruction = decode_one_arg(instruction, opcode, *instr_decoded[1:])
            elif num_args == 2:
                instruction = decode_two_args(instruction, opcode, *instr_decoded[1:])

        disasm.append(instruction + "\n")

    with open(sys.argv[2], "w") as f:
        f.writelines(disasm)


if __name__ == "__main__":
    main()
