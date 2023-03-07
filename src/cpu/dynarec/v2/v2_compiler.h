#ifndef N64_V2_COMPILER_H
#define N64_V2_COMPILER_H

#include <dynarec/dynarec.h>

void print_ir_block();
void v2_compile_new_block(n64_dynarec_block_t *block, bool *code_mask, u64 virtual_address, u32 physical_address);
void v2_compiler_init(n64_dynarec_t* dynarec);

#endif // N64_V2_COMPILER_H