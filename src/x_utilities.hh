#include <X11/Xlib.h>
void xsetupwindow(const char* title, Display*& display, Window& window, GC& gc, int screenwidth = 255, int screenheight = 255);
void xcleanup(Display*& display, Window& window, GC& gc);