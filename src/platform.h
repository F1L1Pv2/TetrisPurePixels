#ifndef PLATFORM
#define PLATFORM

#include <stdbool.h>

bool platform_move_left();
bool platform_move_right();
bool platform_move_down();
bool platform_move_jump_down();

bool platform_rot_left();
bool platform_rot_right();

void platform_draw_rectangle(double x, double y, double width, double height, unsigned int color);

void platform_init(); // here you are supposed to set deltaTime and Time in platform to these pointers
void platform_cleanup();

bool platform_pre_game_update(); //here you are supposed to update delta time and break set to false if you want to close the game
void platform_post_game_update();
double platform_get_deltaTime();
void platform_set_deltaTime(double time);

int platform_screen_width();
int platform_screen_height();

#endif