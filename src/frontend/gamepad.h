#ifndef N64_GAMEPAD_H
#define N64_GAMEPAD_H

#include <system/n64system.h>

void gamepad_init();
void gamepad_refresh();
void gamepad_update_button(byte button, bool state);
void gamepad_update_axis(byte axis, shalf value);
void gamepad_rumble_on(int pif_channel);
void gamepad_rumble_off(int pif_channel);

#endif //N64_GAMEPAD_H
