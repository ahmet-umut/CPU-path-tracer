#pragma once
#include <X11/Xlib.h>
#include "Scene.hh"
#include "vector.hh"
struct Path
{
	vector3 position;	unsigned int color;
	vector<Path> paths;
};
struct Xsystem
{
	Display* display;
	Window window;
	GC gc;
	XImage * image;	unsigned int * imagedata=nullptr;
	Camera camera;
	float xstep, ystep;
	vector3 deep, right, up;
	vector3 screen_center;
	bool triangle_debug=false;
	bool box_debug=false;

	vector<vector3> mirror_path;
	Path tree;
};