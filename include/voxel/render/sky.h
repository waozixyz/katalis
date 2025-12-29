#ifndef SKY_H
#define SKY_H

#include <raylib.h>

void sky_init(void);
void sky_render(Camera3D camera, float time_of_day);
void sky_destroy(void);

#endif // SKY_H
