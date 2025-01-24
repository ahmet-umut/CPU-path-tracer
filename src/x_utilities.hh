#include <X11/Xlib.h>
#include "vector.hh"
void xsetupwindow(const char* title, Display*& display, Window& window, GC& gc, int screenwidth = 255, int screenheight = 255);
void xcleanup(Display*& display, Window& window, GC& gc);
void drawpoint(Display* display, Window window, GC gc, int x, int y, vector4 color);