#include "game_db.h"

static const gamedb_entry_t gamedb[] = {
        {"NNM", "E", SAVE_MEMPAK, "Namco Museum 64"},
        {"NDM", "E", SAVE_MEMPAK, "Doom 64"},
        {"NPD", "E", SAVE_MEMPAK, "Perfect Dark"}
};

#define GAMEDB_SIZE (sizeof(gamedb) / sizeof(gamedb_entry_t))

void gamedb_match(n64_system_t* system) {
    n64_rom_t* rom = &system->mem.rom;
    for (int i = 0; i < GAMEDB_SIZE; i++) {
        bool matches_code = strcmp(gamedb[i].code, rom->code) == 0;
        bool matches_region = false;

        for (int j = 0; j < strlen(gamedb[i].regions) && !matches_region; j++) {
            if (gamedb[i].regions[j] == rom->header.country_code[0]) {
                matches_region = true;
            }
        }

        if (matches_code) {
            if (matches_region) {
                system->mem.save_type = gamedb[i].save_type;
                system->mem.rom.game_name_db = gamedb[i].name;
                logalways("Loaded %s", gamedb[i].name);
                return;
            } else {
                logwarn("Matched code for %s, but not region! Game supposedly exists in regions [%s] but this image has region %c",
                        gamedb[i].name, gamedb[i].regions, rom->header.country_code[0]);
            }
        }

    }

    logwarn("Did not match any Game DB entries. Code: %s Region: %c", system->mem.rom.code, system->mem.rom.header.country_code[0]);

    system->mem.rom.game_name_db = NULL;
    system->mem.save_type = SAVE_NONE;
}
