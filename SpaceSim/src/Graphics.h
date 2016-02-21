#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Window.h"

void initGraphics(int width, int height, WindowHandle window);
void render(double dt);
void present(WindowHandle window);




#endif