#include <sys/resource.h>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <array>
#include <pthread.h>

#include "sendray.hh"
#include "vector_utilities.hh"
#include "hdr.hh"
#include "submit.hh"

#include "state.hh"

using std::cout, std::cin, std::endl, std::string, std::vector, std::flush;

Scene scene;

Xsystem xsystem;
#define xresolution camera.image_resolution[0]
#define yresolution camera.image_resolution[1]
#ifdef _xdebug  // include and global X11 variables
#include "handle_events.hh"
#include "x_utilities.hh"
auto& display = xsystem.display;
auto& window = xsystem.window;
auto& gc = xsystem.gc;
#endif

bool timeoverflow;
string timeof(unsigned long ns)
{
	timeoverflow = false;
	if (ns < 1000)
		return std::to_string(ns / 1) + " nanoseconds";
	else if (ns < 1000000)
		return std::to_string(ns / 1000) + " microseconds";
	else if (ns < 1000000000)
		return std::to_string(ns / 1000000) + " milliseconds";
	else if (ns < 60000000000)
		return std::to_string(ns / 1000000000) + " seconds";
	else if (ns < 3600000000000)
		return std::to_string(ns / 60000000000) + " minutes";
	else if (ns < 86400000000000)
		return std::to_string(ns / 3600000000000) + " hours";
	else
	{
		timeoverflow = true;
		return "more than a day";
	}
}

volatile bool running = false;
std::chrono::_V2::system_clock::time_point t0, t1;
#include <atomic>
std::atomic<int> counter;	int max_count;
void*estimate(void*)
{
	counter = 0;
	while (running)
	{
		sleep(1);	if (!running)	break;
		float x1 = (float)counter / max_count;
		auto time1 = std::chrono::high_resolution_clock::now() - t0;
		sleep(1);	if (!running)	break;
		float x2 = (float)counter / max_count;
		auto time2 = std::chrono::high_resolution_clock::now() - t0;
		
		//t = a(1-e^x) + b
		float dx = x2 - x1;
		float a = (float)(time1*x2/x1 - time2).count() / (1 - exp(x2) - x2/x1 + exp(x1)*x2/x1);
		float b = (time2.count() - a * (1 - exp(x2))) / x2;

		cout << "estimated time of rendering the view: " << timeof(a * (1 - exp(1)) + b) << "                  \r" << flush;
		if (timeoverflow)
		{
			//cout << endl << "estimator debug: a = " << a << " b = " << b << " x1 = " << x1 << " x2 = " << x2 << " time1 = " << time1.count() << " time2 = " << time2.count();
			//cout << flush << "\e[A\r";
		}
	}
	cout << endl;
	return nullptr;
}

int main(int argc, char **argv)
{
	unsigned int camera_index=-1;
	switch (argc)
	{
	case 2:
		break;
	case 3:
		camera_index = atoi(argv[2]);
		break;
	
	default:
		fprintf(stderr, "Usage: %s filename [camera index]\n", argv[0]);
		return 1;
		break;
	}

	// set stack size to 33MB
	const rlim_t kStackSize = 33 * 1024 * 1024;   
    struct rlimit rl;
    int result;
    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
				exit(1);
            }
        }
    }

	vmlSetMode(VML_LA|VML_FTZDAZ_CURRENT);

	auto t_acceleration = std::chrono::high_resolution_clock::now();
    scene = Scene(argv[1]);
	auto t_acceleration_end = std::chrono::high_resolution_clock::now();
	cout << "Parsing the scene and generating acceleration structures took " << timeof((t_acceleration_end - t_acceleration).count()) << endl;

	#ifdef _xdebug
	if (scene.cameras.size() == 1)
		camera_index = 0;
	else if (camera_index == -1)
	{
		cout << "The scene has " << scene.cameras.size() << " cameras. Which one would you like to render?" << endl;
		for (unsigned int i=0; i<scene.cameras.size(); i++)
		{
			cout << i << ": " << scene.cameras[i].image_name << endl;
		}
		cin >> camera_index;
	}

	xsystem.camera = scene.cameras[camera_index];
	auto&camera=xsystem.camera;
	//xresolution = xsystem.camera.image_resolution[0], yresolution = xsystem.camera.image_resolution[1];
	cout << "xresolution: " << xresolution << " yresolution: " << yresolution << endl;
	xsetupwindow(argv[1], xsystem.display, xsystem.window, xsystem.gc, xsystem.camera.image_resolution[0], xsystem.camera.image_resolution[1]);
	xsystem.imagedata = (unsigned int*)malloc(xresolution * yresolution * 4);
	xsystem.image = XCreateImage(xsystem.display, DefaultVisual(xsystem.display, 0), DefaultDepth(xsystem.display, 0), ZPixmap, 0, (char*)xsystem.imagedata, xresolution, yresolution, 32, 0);
	XInitImage(xsystem.image);
	
	xresolution = xsystem.camera.image_resolution[0], yresolution = xsystem.camera.image_resolution[1];
	xsystem.ystep = (-xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3]) / yresolution;
	xsystem.xstep = (-xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1]) / xresolution;

	xsystem.deep = xsystem.camera.gaze;
	xsystem.deep.normalize();
	xsystem.right = xsystem.deep.cross(xsystem.camera.up);
	xsystem.right.normalize();
	xsystem.up = xsystem.right.cross(xsystem.deep);
	xsystem.up.normalize();

	xsystem.screen_center = xsystem.camera.position + xsystem.deep * xsystem.camera.near_distance + xsystem.up*(xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3])/2.f + xsystem.right*(xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1])/2.f;
	if (xsystem.screen_center.x != xsystem.screen_center.x)	{cout << "screen_center.x is nan" << endl;	cout << xsystem.camera.position << endl; cout << xsystem.deep << endl; cout << xsystem.up << endl; cout << xsystem.right << endl;	cout << xsystem.camera.near_distance << endl; cout << xsystem.camera.near_plane[2] << endl; cout << xsystem.camera.near_plane[3] << endl; cout << xsystem.camera.near_plane[0] << endl; cout << xsystem.camera.near_plane[1] << endl; exit(1);}
	//unsigned char* ppmimage = new unsigned char[xresolution * yresolution * 3];

	auto&screen_center=xsystem.screen_center;
	auto&right=xsystem.right;
	auto&up=xsystem.up;
	auto&deep=xsystem.deep;

	while (true)
	#else
	for (auto& camera : scene.cameras)
	#endif
	{
		scene.current_camera = &camera;
		scene.pathtracing = camera.pathtracing;
		static void* imagep;

		#ifdef _xdebug
		usleep(100000/6);
		switch (handle_events(scene, xsystem, imagep))
		{
		case true:
			break;
		case false:
			continue;
		}
		#else
		xsystem.camera = camera;
		//auto&camera=xsystem.camera;
		//xresolution = xsystem.camera.image_resolution[0], yresolution = xsystem.camera.image_resolution[1];
		cout << "xresolution: " << xresolution << " yresolution: " << yresolution << endl;
		//xsetupwindow(argv[1], xsystem.display, xsystem.window, xsystem.gc, xsystem.camera.image_resolution[0], xsystem.camera.image_resolution[1]);
		//xsystem.imagedata = (unsigned int*)malloc(xresolution * yresolution * 4);
		//xsystem.image = XCreateImage(xsystem.display, DefaultVisual(xsystem.display, 0), DefaultDepth(xsystem.display, 0), ZPixmap, 0, (char*)xsystem.imagedata, xresolution, yresolution, 32, 0);
		//XInitImage(xsystem.image);
		
		xresolution = xsystem.camera.image_resolution[0], yresolution = xsystem.camera.image_resolution[1];
		xsystem.ystep = (-xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3]) / yresolution;
		xsystem.xstep = (-xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1]) / xresolution;

		xsystem.deep = xsystem.camera.gaze;
		cblas_sscal(3, 1.f / norm(xsystem.deep), &xsystem.deep, 1);	// normalize gaze
		xsystem.right = xsystem.deep.cross(xsystem.camera.up);
		cblas_sscal(3, 1.f / norm(xsystem.right), &xsystem.right, 1);	// normalize right
		xsystem.up = xsystem.right.cross(xsystem.deep);
		cblas_sscal(3, 1.f / norm(xsystem.up), &xsystem.up, 1);	// normalize up

		xsystem.screen_center = xsystem.camera.position + xsystem.deep * xsystem.camera.near_distance + xsystem.up*(xsystem.camera.near_plane[2] + xsystem.camera.near_plane[3])/2.f + xsystem.right*(xsystem.camera.near_plane[0] + xsystem.camera.near_plane[1])/2.f;
		if (xsystem.screen_center.x != xsystem.screen_center.x)	{cout << "screen_center.x is nan" << endl;	cout << xsystem.camera.position << endl; cout << xsystem.deep << endl; cout << xsystem.up << endl; cout << xsystem.right << endl;	cout << xsystem.camera.near_distance << endl; cout << xsystem.camera.near_plane[2] << endl; cout << xsystem.camera.near_plane[3] << endl; cout << xsystem.camera.near_plane[0] << endl; cout << xsystem.camera.near_plane[1] << endl; exit(1);}
		//unsigned char* ppmimage = new unsigned char[xresolution * yresolution * 3];

		auto&screen_center=xsystem.screen_center;
		auto&right=xsystem.right;
		auto&up=xsystem.up;
		auto&deep=xsystem.deep;
		// TODO: define camera variables such as screen_center, right, up, deep, resolution, etc.
		#endif

		vector3 image[xresolution][yresolution];	imagep = image;

		cout << "--- Rendering Started : " << camera.image_name << " ---" << endl;
		/* static bool __debugger=true;
		if (__debugger)
		{
			__debugger=false;
			continue;
		} */

		max_count = yresolution*xresolution;
		t0 = std::chrono::high_resolution_clock::now(), t1=t0;
		pthread_t thread;
		running = true;
		pthread_create(&thread, nullptr, estimate, nullptr);	pthread_detach(thread);

		float multiplier=1;
		if (camera.pathtracing)
			multiplier = 2 * M_PI;

		#pragma omp parallel
		{
			#pragma omp for nowait
			for (unsigned int y = 0; y < yresolution; y++)
			{
				#ifdef _xdebug
				handle_events(scene, xsystem, image);
				#endif
				for (unsigned int x = 0; x < xresolution; x++)
				{
					vector3 color = {0, 0, 0};
					for (unsigned char sample=0; sample<camera.sample_count; sample++)
					{
						double random_x = drand48()-0.5, random_y = drand48()-0.5;
						vector3 sampled_pixel_position = xsystem.screen_center + right * ((float)x - xresolution/2 + random_x) * xsystem.ystep + -up * ((float)y - yresolution/2 + random_y) * xsystem.xstep;

						if (sampled_pixel_position.x != sampled_pixel_position.x)	{cout << "sampled_pixel_position.x is nan" << endl;	cout << xsystem.screen_center << endl; cout << right << endl; cout << x << endl; cout << random_x << endl; cout << y << endl; cout << random_y << endl; exit(1);}

						vector3 sampled_camera_position = camera.position;
						//cout << "camera position: " << camera.position << endl;
						if (camera.aperture_size > 0)
						{
							/* cout << "aperture size: " << camera.aperture_size << endl;
							cout << "focus distance: " << camera.focus_distance << endl;
							exit(0); */

							float random_angle = drand48() * 2 * M_PI;
							float random_radius = drand48();
							random_radius*=random_radius*camera.aperture_size/2;	//radius is aperture size over 2
							vector3 aperture_sample = right * random_radius * cos(random_angle) + up * random_radius * sin(random_angle);
							sampled_camera_position = camera.position + aperture_sample;
							//cout << "aperture sample: " << aperture_sample << endl;
						}

						vector3 target = (sampled_pixel_position +- camera.position) * (1 + camera.focus_distance / camera.near_distance) + camera.position;

						if (target.x != target.x)	{cout << "target.x is nan" << endl; cout << sampled_pixel_position << endl; cout << camera.position << endl;
							exit(1);}
						if (target.y != target.y)	{cout << "target.y is nan" << endl; exit(1);}
						if (target.z != target.z)	{cout << "target.z is nan" << endl; exit(1);}

						Ray ray = {sampled_camera_position, target-sampled_camera_position};
						vector3 sampled_color = sendray(scene, ray, false,0,0, {x,y,true});
						color = color + sampled_color;
					}
					color = color / camera.sample_count * multiplier;
					if (!camera.hdr)
						color = clamp(color);
					image[x][y] = color;
					#ifdef _xdebug
					xsystem.imagedata[y * xresolution + x] = ((unsigned int)color.x<<16) + ((unsigned int)color.y<<8) + (unsigned int)color.z;
					#endif

					counter++;
				}
				#ifdef _xdebug
				XImage*image = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, 0), ZPixmap, 0, (char*)(xsystem.imagedata + y*xresolution), xresolution, 1, 32, 0);
				XInitImage(image);
				XPutImage(display, window, gc, image, 0, 0, 0, y, xresolution, 1);
				#endif
			}
		}
		running = false;
		t1 = std::chrono::high_resolution_clock::now();

		cout << endl << endl << "Rendering the view took " << timeof((t1-t0).count()) << endl;

		if (camera.hdr)
		{
			cout << "HDR post-processing..." << endl;
			string exr_name = camera.image_name;
			//saveToEXR(image, xresolution, yresolution, "exr/" + camera.image_name);	//save before tone-mapping
			//saveToEXR(image, xresolution, yresolution, camera.image_name);	//save before tone-mapping
			applyHDRTonemapping(xsystem, xresolution, yresolution, image);
			#ifdef _xdebug
			XPutImage(display, window, gc, xsystem.image, 0, 0, 0, 0, xresolution, yresolution);
			#else
			cout << "HDR post-processing ended." << endl;
			#endif
		}
		string png_name /*name can end with .exr, then convert it to .png*/ = camera.image_name;
		if (camera.hdr)
			png_name = png_name.substr(0, png_name.size()-4) + ".png";
		//saveToPNG(image, xresolution, yresolution, png_name);
		cout << "--- Rendering Ended ---" << endl;
	}
}