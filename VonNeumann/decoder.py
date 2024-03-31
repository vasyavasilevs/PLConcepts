import numpy as np

import isa

def decode(instr: np.uint32) -> tuple[int, ...]:
    binary: str = format(int(instr), '032b')
    opcode = int(binary[0:5], 2)

    if isa.is_reg_reg_instr(opcode):
        # [0:4][5:7][8:11][12:14][15:18][:31]
        reg1_access_level = int(binary[5:8], 2)
        reg1_value = int(binary[8:12], 2)
        reg2_access_level = int(binary[12:15], 2)
        reg2_value = int(binary[15:19], 2)
        return opcode, reg1_access_level, reg1_value, reg2_access_level, reg2_value
    if isa.is_reg_imm_instr(opcode):
        # [0:4][5:7][8:11][12:31]
        reg_access_level = int(binary[5:8], 2)
        reg_value = int(binary[8:12], 2)
        imm = int(binary[12:], 2)
        return opcode, reg_access_level, reg_value, imm
    if isa.is_reg_instr(opcode):
        # [0:4][5:7][8:11][:31]
        reg_access_level = int(binary[5:8], 2)
        reg_value = int(binary[8:12], 2)
        imm = int(binary[12:], 2)
        return opcode, reg_access_level, reg_value
    if isa.is_imm_instr(opcode):
        # [0:4][5:31]
        imm = int(binary[5:], 2)
        return opcode, imm
    if isa.is_opcode_only_instr(opcode):
        # [0:4][:31]
        return (opcode,)

    raise RuntimeError("Unknown instruction: " + binary)
