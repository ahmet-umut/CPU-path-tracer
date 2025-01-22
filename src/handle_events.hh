#pragma once
#include <X11/Xlib.h>
#include "Scene.hh"
#include "XSystem.hh"
bool handle_events(Scene&scene, Xsystem&xsystem, void*pointer=nullptr);