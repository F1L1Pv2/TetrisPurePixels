#ifndef PLATFORM
#define PLATFORM

#include <stdbool.h>

static double* deltaTime;
static double* Time;

bool platform_move_left();
bool platform_move_right();
bool platform_move_down();
bool platform_move_jump_down();

bool platform_rot_left();
bool platform_rot_right();

void platform_draw_rectangle(double x, double y, double width, double height, unsigned int color);

void platform_init(double* deltaTimeIN, double* TimeIN);
void platform_cleanup();

bool platform_events();
void platform_update();

int platform_screen_width();
int platform_screen_height();

#endif