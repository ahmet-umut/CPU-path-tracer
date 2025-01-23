#include "handle_events.hh"
#include <X11/Xlib.h>
#include <stdlib.h>
#include <iostream>
#include "Scene.hh"
#include "Ray.hh"
#include "sendray.hh"
#include "vector_utilities.hh"
#include "test.hh"
#include "submit.hh"

#define xresolution scene.current_camera->image_resolution[0]
#define yresolution scene.current_camera->image_resolution[1]

void draw(Xsystem&xsystem, Path&path)
{
	for (auto&p:path.paths)
	{
		XSetForeground(xsystem.display, xsystem.gc, p.color);
		XDrawLine(xsystem.display, xsystem.window, xsystem.gc, path.position.x, path.position.y, p.position.x, p.position.y);
		draw(xsystem, p);
	}
}

using namespace std;
bool handle_events(Scene&scene, Xsystem&xsystem, void*imagepointer)
{
	auto& display = xsystem.display;
	auto& window = xsystem.window;
	auto& gc = xsystem.gc;

    XEvent event;

	static int posx=-1, posy=-1;

    // Process events
    while (XPending(display)) {
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose:break;
            case KeyPress: {
                // Exit on any key press
                switch (event.xkey.keycode)
				{
				case 9: // Escape key
					exit(0);
					break;
				case 65: //space
					return true;
					break;
				case 36: // Enter key
					XPutImage(display, window, gc, xsystem.image, 0, 0, 0, 0, xresolution, yresolution);
					break;
				case 111: // Up arrow
					break;
				case 116: // Down arrow
					/* static auto iterator = scene.meshes.begin();
					if (iterator == scene.meshes.end())	iterator = scene.meshes.begin();
					cout << "event log -";
					cout << "directing camera to mesh " << iterator->say() << endl;
					//global_camera.gaze = iterator->center - global_camera.position;
					global_camera.gaze = iterator->data->box.center() - global_camera.position;
					global_camera.gaze.normalize();
					deep = global_camera.gaze;
					right = deep.cross(global_camera.up).normalized();
					up = right.cross(deep).normalized();
					screen_center = global_camera.position + deep;
					iterator++; */
					//return true;
					break;
				// right arrow
				case 114:
					break;
				case 46:	// L key
					break;
				case 0x28:	//D key
					for (auto&vertex:scene.vertices)
					{
						auto pixel = test(xsystem, vertex);
						XSetForeground(display, gc, 0x00FF00);
						XDrawPoint(display, window, gc, pixel.x, pixel.y);
					}
					break;

				case 0x27:	//S key
					//saveToPNG(imagepointer, xresolution, yresolution, "png/" + scene.current_camera->image_name);
					break;

				case 0x21:	//P key
					saveToPNG(imagepointer, xresolution, yresolution, "png/" + scene.current_camera->image_name);
					break;
				case 43:	// H key
					//sa
					break;
				}
                break;
            }
            case ButtonPress: {
                // Example: Log mouse click position
				switch (event.xkey.keycode)
				{
				case 1: // Left mouse button
					cout << endl << "testing pixel " << event.xbutton.x << " " << event.xbutton.y << endl;
					{
						auto x = event.xbutton.x, y = event.xbutton.y;
						posx = x, posy = y;
						float ystep = (-xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3]) / yresolution;
						float xstep = (-xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1]) / xresolution;
						vector3 direction = xsystem.screen_center + xsystem.right * ((float)x - xresolution/2) * ystep +- xsystem.up * ((float)y - yresolution/2) * xstep - xsystem.camera.position;
						Ray ray = {xsystem.camera.position, direction};

						xsystem.tree.position = {x,y,true};
						xsystem.tree.paths.clear();

						sendray(scene,ray,true, 0,0, {x,y,true}, &xsystem, &xsystem.tree);
						draw(xsystem, xsystem.tree);

						//xsystem.mirror_path.clear();
						/* XPoint points[xsystem.mirror_path.size()];
						for (int i=0; i<xsystem.mirror_path.size(); i++)
						{
							points[i].x = xsystem.mirror_path[i].x;
							points[i].y = xsystem.mirror_path[i].y;
						}
						XSetForeground(display, gc, 0x002F00);
						XDrawLines(display, window, gc, points, xsystem.mirror_path.size(), CoordModeOrigin); */

						cout << "------------------------------------------------" << endl;
						int yres = yresolution;
						if (imagepointer == nullptr)	break;
						auto image = (vector3(*)[yres])imagepointer;
						if (xsystem.camera.hdr)	cout << "processed image: " << image[x][y] << endl;
					}
					break;
				}
				break;
            }
			case ButtonRelease: {
				switch (event.xkey.keycode)
				{
				case 1: // Left mouse button
					cout << "button released" << endl;
					/* if (abs(posx-event.xbutton.x) < 10 && abs(posy-event.xbutton.y) < 10)	break;
					cout << endl << "screen is changing " << event.xbutton.x << " " << event.xbutton.y << endl;
					{
						auto minx = min(posx, event.xbutton.x), maxx = max(posx, event.xbutton.x);
						auto miny = min(posy, event.xbutton.y), maxy = max(posy, event.xbutton.y);
						auto width = xsystem.camera.near_plane[1] - xsystem.camera.near_plane[0];
						auto height = xsystem.camera.near_plane[3] - xsystem.camera.near_plane[2];
						xsystem.camera.near_plane[0] += width * minx / xresolution;
						xsystem.camera.near_plane[1] -= width * (xresolution - maxx) / xresolution;
						xsystem.camera.near_plane[2] += height * miny / yresolution;
						xsystem.camera.near_plane[3] -= height * (yresolution - maxy) / yresolution;
						xsystem.camera.image_resolution[0] = (maxx - minx) * 2;
						xsystem.camera.image_resolution[1] = (maxy - miny) * 2;
						xsystem.xstep = (-xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1]) / xresolution;
						xsystem.ystep = (-xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3]) / yresolution;
						xsystem.camera.sample_count=2;
						xsystem.imagedata = (unsigned int*)malloc(xresolution * yresolution * 4);
						xsystem.image = XCreateImage(xsystem.display, DefaultVisual(xsystem.display, 0), DefaultDepth(xsystem.display, 0), ZPixmap, 0, (char*)xsystem.imagedata, xresolution, yresolution, 32, 0);
						XInitImage(xsystem.image);
						xsystem.screen_center = xsystem.camera.position + xsystem.deep * xsystem.camera.near_distance + xsystem.up*(xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3])/2.f + xsystem.right*(xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1])/2.f;
						cout << "screen is changed" << endl;
					} */
					break;
				}
				break;
			}
        }
    }

	return false;
}