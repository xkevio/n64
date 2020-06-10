#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"
#include "mips32.h"

#define MIPS32_CP    0b010000
#define MIPS32_LUI   0b001111
#define MIPS32_ADDIU 0b001001
#define MIPS32_LW    0b100011

#define MTC0_MASK  0b11111111111000000000011111111111
#define MTC0_VALUE 0b01000000100000000000000000000000

mips32_instruction_type_t decode(r4300i_t* cpu, mips32_instruction_t instr) {
    char buf[50];
    disassemble32(cpu->pc, instr.raw, buf, 50);
    logdebug("[0x%08X] %s", cpu->pc, buf)
    switch (instr.op) {
        case MIPS32_CP:
            if ((instr.raw & MTC0_MASK) == MTC0_VALUE) {
                return MTC0;
            } else {
                logfatal("other/unknown MIPS32 Coprocessor")
            }
        case MIPS32_LUI:
            return LUI;
        case MIPS32_ADDIU:
            return ADDIU;
        case MIPS32_LW:
            return LW;
        default:
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
    }
}

void r4300i_step(r4300i_t* cpu, word instruction) {
    mips32_instruction_t parsed;
    parsed.raw = instruction;

    switch (decode(cpu, parsed)) {
        case MTC0:
            mtc0(cpu, parsed);
            break;
        case LUI:
            lui(cpu, parsed);
            break;
        case ADDIU:
            addiu(cpu, parsed);
            break;
        case LW:
            lw(cpu, parsed);
        default:
            logfatal("Unknown instruction type!")
    }

    cpu->pc += 4;
}
