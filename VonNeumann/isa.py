# ISA - Instruction Set Architecture


# The following types instructions are supported in VM:

# Opcode-only instructions:
# Opcode: 5 bits

# Instructions with one register:
# Opcode: 5 bits
# Arg1: 3 bits + 4 bits

# Instructions with two registers:
# Opcode: 5 bits
# Arg1: 3 bits + 4 bits
# Arg2: 3 bits + 4 bits

# Instruction with one immediate:
# Opcode: 5 bits
# Arg1: 27 bits

# Instruction with a register and immediate:
# Opcode: 5 bits
# Arg1: 3 bits + 4 bits
# Arg2: 20 bits


NAME_TO_OPCODE = {
    "MOV": 0,
    "ADD": 1,
    "SUB": 2,
    "POP": 3,
    "PUSH": 4,
    "CALL": 5,
    "FBEGIN": 6,
    "FEND": 7,
    "TERM": 8,
    "JUMP": 9,
    "JUMP_IF_GZ": 10,
    "PRINT": 11,
    "READ": 12,
    "PRINTSTR": 13,
    "LOADL": 14,
    "LOADH": 15,
    "ADDI": 16,
    "SUBI": 17,
    "INDIR_CALL": 18,
}

OPCODE_TO_NAME = [
    "MOV",
    "ADD",
    "SUB",
    "POP",
    "PUSH",
    "CALL",
    "FBEGIN",
    "FEND",
    "TERM",
    "JUMP",
    "JUMP_IF_GZ",
    "PRINT",
    "READ",
    "PRINTSTR",
    "LOADL",
    "LOADH",
    "ADDI",
    "SUBI",
    "INDIR_CALL",
]

OPCODE_TO_NUM_ARGS = [
    2,
    2,
    2,
    0,
    1,
    1,
    1,
    0,
    0,
    1,
    2,
    1,
    1,
    1,
    2,
    2,
    2,
    2,
    1,
]

SPECIAL_REGISTERS = {"ip": 0, "sp": 1, "rv": 2}
SPECIAL_REGISTERS_TO_NAME = ["ip", "sp", "rv"]
IP_INDEX = 0
SP_INDEX = 1
MAX_REGISTERS = 16


def is_reg(addr: int) -> bool:
    return addr >= 0 and addr < MAX_REGISTERS


_REG_REG_INSTRS = set([
    NAME_TO_OPCODE["MOV"],
    NAME_TO_OPCODE["ADD"],
    NAME_TO_OPCODE["SUB"],
])

def is_reg_reg_instr(opcode: int) -> bool:
    return opcode in _REG_REG_INSTRS


_REG_IMM_INSTRS = set([
    NAME_TO_OPCODE["ADDI"],
    NAME_TO_OPCODE["SUBI"],
    NAME_TO_OPCODE["LOADL"],
    NAME_TO_OPCODE["LOADH"],
    NAME_TO_OPCODE["JUMP_IF_GZ"],
])

def is_reg_imm_instr(opcode: int) -> bool:
    return opcode in _REG_IMM_INSTRS


_REG_INSTRS = set([
    NAME_TO_OPCODE["PUSH"],
    NAME_TO_OPCODE["JUMP"],
    NAME_TO_OPCODE["PRINT"],
    NAME_TO_OPCODE["READ"],
    NAME_TO_OPCODE["INDIR_CALL"],
])

def is_reg_instr(opcode: int) -> bool:
    return opcode in _REG_INSTRS


_IMM_INSTRS = set([
    NAME_TO_OPCODE["CALL"],
    NAME_TO_OPCODE["FBEGIN"],
    NAME_TO_OPCODE["PRINTSTR"],
])

def is_imm_instr(opcode: int) -> bool:
    return opcode in _IMM_INSTRS


_OPCODE_ONLY_INSTRS = set([
    NAME_TO_OPCODE["POP"],
    NAME_TO_OPCODE["TERM"],
    NAME_TO_OPCODE["FEND"],
])

def is_opcode_only_instr(opcode: int) -> bool:
    return opcode in _OPCODE_ONLY_INSTRS
