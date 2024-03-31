import numpy as np
import sys

import decoder
import isa


class Memory:
    def __init__(self, memory_size):
        self.memory = np.zeros(memory_size, dtype=np.int32)

    def write_word(self, address: int, word: np.int32):
        self.memory[address] = word

    def read_word(self, address: int) -> np.int32:
        return self.memory[address]


class Interpreter:
    def __init__(self, memory, static_offset):
        self.memory: Memory = memory
        self.static_offset = static_offset
        self.function_startpoints = {}
        self.reading_function = False
        self._debug: bool = False

        self._dispatch_table = [
            self.mov,
            self.add,
            self.sub,
            self.pop,
            self.push,
            self.call,
            self.fbegin,
            self.fend,
            self.terminate,
            self.jump,
            self.jump_if_gz,
            self.print_,
            self.read,
            self.print_str,
            self.loadl,
            self.loadh,
            self.addi,
            self.subi,
            self.indir_call,
        ]

    def ip_value(self):
        return self.memory.read_word(isa.IP_INDEX)

    def sp_value(self):
        return self.memory.read_word(isa.SP_INDEX)

    def get_value(self, value, access) -> np.int32:
        for _ in range(access):
            value = self.memory.read_word(value)
        return value

    def next_command(self):
        self.memory.write_word(isa.IP_INDEX, self.memory.read_word(isa.IP_INDEX) + 1)

    def mov(self, dest_access, dest, src_access, src):
        assert isa.is_reg(dest)
        assert isa.is_reg(src)

        dest_address = self.get_value(dest, dest_access)
        src_value = self.get_value(src, src_access)
        if self._debug:
            print(f"[DEBUG] MOV: res_addr={dest_address}, value={src_value}")
        self.memory.write_word(dest_address, src_value)
        self.next_command()
        return True

    def add(self, dest_access, dest, term_access, term):
        assert isa.is_reg(dest)
        assert isa.is_reg(term)

        dest_address = self.get_value(dest, dest_access)
        addition_value = self.get_value(term, term_access)
        add_result = self.memory.read_word(dest) + addition_value
        if self._debug:
            print(f"[DEBUG] ADD: res_addr={dest_address}, value={add_result}")
        self.memory.write_word(dest_address, add_result)
        self.next_command()
        return True

    def sub(self, dest_access, dest, sub_access, sub_v):
        assert isa.is_reg(dest)
        assert isa.is_reg(sub_v)

        dest_address = self.get_value(dest, dest_access)
        sub_value = self.get_value(sub_v, sub_access)
        sub_result = self.memory.read_word(dest) - sub_value
        self.memory.write_word(dest_address, sub_result)
        if self._debug:
            print(f"[DEBUG] SUB: res_addr={dest_address}, value={sub_result}")
        self.next_command()
        return True

    def jump(self, dest_access, dest):
        assert isa.is_reg(dest)

        instruction_number = self.get_value(dest, dest_access)
        if self._debug:
            print(f"[DEBUG] JUMP: value={instruction_number}")
        self.memory.write_word(isa.IP_INDEX, instruction_number)
        return True

    def jump_if_gz(self, val_access, val, imm):
        assert isa.is_reg(val)

        value = self.get_value(val, val_access)
        if self._debug:
            print(f"[DEBUG] JUMP_IF_GZ: offset={imm}, value={value}")
        if value > 0:
            self.addi(0, isa.IP_INDEX, imm, advance=False)
        else:
            self.next_command()
        return True

    def pop(self):
        self.addi(0, isa.SP_INDEX, 1, advance=False)
        self.next_command()
        return True

    def push(self, val_access, val, advance: bool = True):
        # self.subi(0, isa.SP_INDEX, 1, advance=False)
        self.memory.write_word(isa.SP_INDEX, self.memory.read_word(isa.SP_INDEX) - 1)

        self.memory.write_word(self.sp_value(), self.get_value(val, val_access))

        if advance:
            self.next_command()
        return True

    def call(self, function_number):
        self.push(0, self.ip_value() + 1, advance=False)
        self.memory.write_word(isa.IP_INDEX, self.function_startpoints[function_number])
        return True

    def fbegin(self, function_number):
        if self.reading_function:
            raise RuntimeError("Nested functions are not implemented")

        self.function_startpoints[function_number] = self.ip_value() + 1
        self.reading_function = True
        self.next_command()
        return True
    
    def fend(self):
        self.reading_function = False
        self.next_command()
        return True

    def print_(self, val_access, val):
        value = self.get_value(val, val_access)
        if self._debug:
            print(f"[DEBUG] print_: value={value}, address={val}, val_access={val_access}")
        print(value)
        self.next_command()
        return True

    def read(self, dest_access, dest):
        address = self.get_value(dest, dest_access)
        read_value = int(input())
        self.memory.write_word(address, read_value)
        self.next_command()
        return True

    def print_str(self, arg):
        str_address = self.memory.read_word(self.static_offset + arg)
        str_len = self.memory.read_word(self.static_offset + str_address)

        str = ""
        for i in range(str_len):
            str += chr(self.memory.read_word(self.static_offset + str_address + i + 1))

        print(str)
        self.next_command()
        return True

    def loadl(self, dest_access, dest, imm: int):
        assert isa.is_reg(dest)
        assert imm >= 0 and imm < 2 ** 16

        dest_address = self.get_value(dest, dest_access)

        prev = self.memory.read_word(dest_address)
        prev_bin = format(prev, "032b")
        imm_bin = format(imm, "016b")
        new_bin = prev_bin[0:16] + imm_bin
        new_value = int(new_bin, 2)
        self.memory.write_word(dest_address, new_value)

        if self._debug:
            print(f"[DEBUG] LOADL: res_addr={dest_address}, value={imm}, result={new_value}")

        self.next_command()
        return True

    def loadh(self, dest_access, dest, imm: int):
        assert isa.is_reg(dest)
        assert imm >= 0 and imm < 2 ** 16

        dest_address = self.get_value(dest, dest_access)

        prev = self.memory.read_word(dest_address)
        prev_bin = format(prev, "032b")
        imm_bin = format(imm, "016b")
        new_bin = imm_bin + prev_bin[16:]
        new_value = int(new_bin, 2)
        self.memory.write_word(dest_address, new_value)

        if self._debug:
            print(f"[DEBUG] LOADH: res_addr={dest_address}, value={imm}, result={new_value}")

        self.next_command()
        return True

    def addi(self, dest_access, dest, term: int, advance: bool = True):
        assert isa.is_reg(dest)

        dest_address = self.get_value(dest, dest_access)
        add_result = self.memory.read_word(dest) + term
        if self._debug:
            print(f"[DEBUG] ADDI: res_addr={dest_address}, value={add_result}")
        self.memory.write_word(dest_address, add_result)

        if advance:
            self.next_command()
        return True

    def subi(self, dest_access, dest, sub_v: int, advance: bool = True):
        assert isa.is_reg(dest)

        dest_address = self.get_value(dest, dest_access)
        sub_result = self.memory.read_word(dest) - sub_v
        if self._debug:
            print(f"[DEBUG] SUBI: res_addr={dest_address}, sub_result={sub_result}")
        self.memory.write_word(dest_address, sub_result)

        if advance:
            self.next_command()
        return True

    def indir_call(self, val_access, val):
        function_number = self.get_value(val, val_access)
        self.push(0, self.ip_value() + 1, advance=False)
        self.memory.write_word(isa.IP_INDEX, self.function_startpoints[function_number])
        return True

    def terminate(self):
        return False

    def dispatch(self) -> bool:
        """
        Returns False on termination.
        """
        instruction = np.uint32(self.get_value(self.ip_value(), 1))
        instr_decoded: tuple[int, ...] = decoder.decode(instruction)
        opcode: int = instr_decoded[0]

        if self._debug:
            print(f"[DEBUG] #{self.ip_value()} {isa.OPCODE_TO_NAME[opcode]}: {format(instruction, '032b')}")

        # Need to skip all functions until IP reaches main code
        if self.reading_function and opcode != isa.NAME_TO_OPCODE["FEND"]:
            self.next_command()
            return True

        assert opcode >= 0 and opcode < len(self._dispatch_table)
        handler = self._dispatch_table[opcode]
        return handler(*instr_decoded[1:])

    def execute(self, debug: bool = False):
        prev_debug = self._debug
        self._debug = debug
        while self.dispatch():
            pass
        self._debug = prev_debug


def main():
    if len(sys.argv) < 2:
        raise RuntimeError("Specify BIN file")

    debug_mode = (len(sys.argv) == 3 and sys.argv[2] == "DEBUG")

    memory_size = 10000000
    memory = Memory(memory_size)
    memory.write_word(isa.IP_INDEX, isa.MAX_REGISTERS)
    memory.write_word(isa.SP_INDEX, memory_size)

    code = np.fromfile(sys.argv[1], dtype=np.int32)
    code_size = code[0]
    if debug_mode:
        print(f"[DEBUG] Loaded {code_size} instructions")

    for i in range(1, len(code)):
        memory.write_word(isa.MAX_REGISTERS + i - 1, code[i])

    interpreter = Interpreter(memory, isa.MAX_REGISTERS + code_size)
    interpreter.execute(debug=debug_mode)


if __name__ == "__main__":
    main()
