#include <stdio.h>
#include <stdlib.h>
#define LOG_ENABLED
#include <log.h>
#include <system/n64system.h>
#include <cpu/mips_instructions.h>
#include <cpu/r4300i_register_access.h>
#include <mem/n64bus.h>

#define MAX_STEPS 10000000
#define TEST_FAILED_REGISTER 30

bool test_complete() {
    sdword test_failed = get_register(TEST_FAILED_REGISTER);
    if (test_failed != 0) {
        if (test_failed != -1) {
            logfatal("Test #%ld failed.", test_failed);
        }

        return true;
    } else {
        return false;
    }
}

int main(int argc, char** argv) {
    if (argc == 0) {
        logfatal("Pass me a ROM file please");
    }

    log_set_verbosity(LOG_VERBOSITY_DEBUG);

    init_n64system(argv[1], false, false, UNKNOWN_VIDEO_TYPE, false);
    // Normally handled by the bootcode, we gotta do it ourselves.
    for (int i = 0; i < 1048576; i++) {
        byte b = n64_read_physical_byte(0x10001000 + i);
        n64_write_physical_byte(0x00001000 + i, b);
    }

    set_pc_word_r4300i(n64sys.mem.rom.header.program_counter);

    loginfo("Initial PC: 0x%016lX\n", N64CPU.pc);

    int steps = 0;
    for (; steps < MAX_STEPS && !test_complete(); steps++) {
        n64_system_step(true);
    }

    if (!test_complete()) {
        logfatal("Test timed out after %d steps\n", MAX_STEPS);
    }

    printf("SUCCESS: all tests passed! Took %d steps.\n", steps);
}