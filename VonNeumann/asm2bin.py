import numpy as np
import sys

import isa

# Instructions with two registers:
# Opcode: 5 bits
# Arg1: 3 bits + 4 bits
# Arg2: 3 bits + 4 bits

def _encode_reg_reg(opcode: str, reg1: str, reg2: str)-> np.uint32:
    opcode = format(isa.NAME_TO_OPCODE[opcode], "05b")
    access_level1, reg_index1 = get_encoded_reg_with_access_level(reg1)
    access_level2, reg_index2 = get_encoded_reg_with_access_level(reg2)
    encoded = opcode + access_level1 + reg_index1 + access_level2 + reg_index2
    encoded += "0" * (32 - len(encoded))
    return np.uint32(int(encoded, 2))

def encode_mov(reg1: str, reg2: str)-> np.uint32:
    return _encode_reg_reg("MOV", reg1, reg2)

def encode_add(reg1: str, reg2: str)-> np.uint32:
    return _encode_reg_reg("ADD", reg1, reg2)

def encode_sub(reg1: str, reg2: str)-> np.uint32:
    return _encode_reg_reg("SUB", reg1, reg2)


# Instruction with a register and immediate:
# Opcode: 5 bits
# Arg1: 3 bits + 4 bits
# Arg2: 20 bits

def _encode_reg_imm(opcode: str, reg: str, imm: str)-> np.uint32:
    opcode = format(isa.NAME_TO_OPCODE[opcode], "05b")
    access_level, reg_index = get_encoded_reg_with_access_level(reg)
    imm = get_imm(imm, 20)
    encoded = opcode + access_level + reg_index + imm
    return np.uint32(int(encoded, 2))

def encode_addi(reg: str, imm: str)-> np.uint32:
    return _encode_reg_imm("ADDI", reg, imm)

def encode_subi(reg: str, imm: str)-> np.uint32:
    return _encode_reg_imm("SUBI", reg, imm)

def encode_loadl(reg: str, imm: str)-> np.uint32:
    return _encode_reg_imm("LOADL", reg, imm)

def encode_loadh(reg: str, imm: str)-> np.uint32:
    return _encode_reg_imm("LOADH", reg, imm)

def encode_jump_if_gz(reg: str, imm: str)-> np.uint32:
    return _encode_reg_imm("JUMP_IF_GZ", reg, imm)


# Instructions with one register:
# Opcode: 5 bits
# Arg1: 3 bits + 4 bits

def _encode_reg(opcode: str, reg: str)-> np.uint32:
    opcode = format(isa.NAME_TO_OPCODE[opcode], "05b")
    access_level, reg_index = get_encoded_reg_with_access_level(reg)
    encoded = opcode + access_level + reg_index
    encoded += "0" * (32 - len(encoded))
    return np.uint32(int(encoded, 2))

def encode_push(reg: str)-> np.uint32:
    return _encode_reg("PUSH", reg)

def encode_jump(reg: str)-> np.uint32:
    return _encode_reg("JUMP", reg)

def encode_print(reg: str)-> np.uint32:
    return _encode_reg("PRINT", reg)

def encode_read(reg: str)-> np.uint32:
    return _encode_reg("READ", reg)

def encode_indir_call(reg: str) -> np.uint32:
    return _encode_reg("INDIR_CALL", reg)


# Instruction with one immediate:
# Opcode: 5 bits
# Arg1: 27 bits

def _encode_imm(opcode: str, imm: str)-> np.uint32:
    opcode = format(isa.NAME_TO_OPCODE[opcode], "05b")
    imm = get_imm(imm, 27)
    encoded = opcode + imm
    return np.uint32(int(encoded, 2))

def encode_call(imm: str)-> np.uint32:
    return _encode_imm("CALL", imm)

def encode_fbegin(imm: str)-> np.uint32:
    return _encode_imm("FBEGIN", imm)

def encode_printstr(imm: str)-> np.uint32:
    return _encode_imm("PRINTSTR", imm)


# Opcode-only instructions:
# Opcode: 5 bits

def _encode_opcode_only(opcode: str)-> np.uint32:
    opcode = format(isa.NAME_TO_OPCODE[opcode], "05b")
    opcode += "0" * (32 - len(opcode))
    return np.uint32(int(opcode, 2))

def encode_pop()-> np.uint32:
    return _encode_opcode_only("POP")

def encode_term()-> np.uint32:
    return _encode_opcode_only("TERM")

def encode_fend()-> np.uint32:
    return _encode_opcode_only("FEND")


ENCODERS = {
    "MOV": encode_mov,
    "ADD": encode_add,
    "SUB": encode_sub,
    "POP": encode_pop,
    "PUSH": encode_push,
    "CALL": encode_call,
    "FBEGIN": encode_fbegin,
    "FEND": encode_fend,
    "TERM": encode_term,
    "JUMP": encode_jump,
    "JUMP_IF_GZ": encode_jump_if_gz,
    "PRINT": encode_print,
    "READ": encode_read,
    "PRINTSTR": encode_printstr,
    "LOADL": encode_loadl,
    "LOADH": encode_loadh,
    "ADDI": encode_addi,
    "SUBI": encode_subi,
    "INDIR_CALL": encode_indir_call,
}

# Helper functions

def decode_reg(reg: str) -> int | None:
    if reg in isa.SPECIAL_REGISTERS:
        return isa.SPECIAL_REGISTERS[reg]

    if reg[0] == "r":
        reg_number = 0
        try:
            reg_number = int(reg[1:])
        except ValueError:
            pass
        if reg_number and reg_number <= isa.MAX_REGISTERS - len(isa.SPECIAL_REGISTERS):
            return len(isa.SPECIAL_REGISTERS) + reg_number - 1
    return None


def get_imm(imm: str, bits: int) -> str:
    imm = int(imm)
    if imm < 0 or imm >= 2 ** bits:
        raise RuntimeError("Invalid immediate: " + imm)
    return format(imm, f"0{bits}b")


def _get_reg_with_access_level(arg) -> tuple[int, int]:
    access_level = 0
    while arg[access_level] == "*":
        access_level += 1
    assert access_level < 2 ** 3

    reg_index = decode_reg(arg[access_level:])
    assert reg_index is not None

    return access_level, reg_index


def get_encoded_reg_with_access_level(arg: str) -> tuple[str, str]:
    access_level, reg_index = _get_reg_with_access_level(arg)
    access_level = format(access_level, "03b")
    reg_index = format(reg_index, "04b")
    return access_level, reg_index


def generate_string_table(string_array: list[str]):
    """
    Strings table is filled as follows:
    [enumerated offsets to strings] + [strings with prepended lengths]

    Example:
    We want to fill two string: "abc" and "qwerty"
    The corresponding string table will be:
    [2, 6, 3, 'a', 'b', 'c', 5, 'q', 'w', 'e', 'r', 't', 'y']
          ^
          |
    the end of offsets section   
    """
    # reserve space for indexes-to-offset mapping
    total_size = len(string_array)
    for str in string_array:
        total_size += len(str) + 1

    data = np.zeros(total_size, dtype=np.uint32)

    current_offset = len(string_array)
    for i in range(len(string_array)):
        str = string_array[i]
        data[i] = current_offset
        data[current_offset] = len(str)
        current_offset += 1
        for c in str:
            code = ord(c)
            data[current_offset] = code
            current_offset += 1

    return data


def generate_instructions(source_filename: str) -> tuple[np.ndarray, list[str]]:
    instructions: list[np.uint32] = []
    string_array: list[str] = []

    with open(source_filename) as f:
        for line in f.readlines():
            if len(line) == 0:
                continue

            tokens_high_level: list[str] = line.strip().split(' ', 1)
            if tokens_high_level[0] == "PRINTSTR":
                string_array.append(tokens_high_level[1])
                instructions.append(encode_printstr(len(string_array) - 1))
                continue

            tokens: list[str] = line.strip().split(' ')
            if tokens[0] not in ENCODERS:
                print("Invalid command:", tokens[0], "in line:", line)
                raise RuntimeError()

            encoder = ENCODERS[tokens[0]]
            instr = encoder(*tokens[1:])
            instructions.append(instr)

    return np.array(instructions, dtype=np.uint32), string_array


def main():
    if len(sys.argv) < 1:
        raise RuntimeError("Specify ASM file")

    if len(sys.argv) < 2:
        raise RuntimeError("Specify BIN filename")

    instructions, string_array = generate_instructions(sys.argv[1])
    instrs_count = len(instructions)
    static_data = generate_string_table(string_array)

    total_result: np.ndarray = np.concatenate([
        np.array([instrs_count], dtype=np.uint32),
        instructions,
        static_data
    ])
    total_result.tofile(sys.argv[2])


if __name__ == "__main__":
    main()
