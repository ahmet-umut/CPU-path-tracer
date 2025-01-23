#include <sys/resource.h>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <array>
#include <pthread.h>
#include <thread>

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

#include <atomic>
using std::atomic;

atomic<int> totalsamples;
int pendingsamples=0;

volatile bool running = false;
std::chrono::_V2::system_clock::time_point t0, t1;
std::atomic<int> counter;	int max_count;
void*estimate(void*)
{
	counter = 0;
	while (running)
	{
		sleep(1);	if (!running)	break;
		float x1 = (float)totalsamples / max_count;
		auto time1 = std::chrono::high_resolution_clock::now() - t0;
		sleep(1);	if (!running)	break;
		float x2 = (float)totalsamples / max_count;
		auto time2 = std::chrono::high_resolution_clock::now() - t0;
		
		//t = a(1-e^x) + b
		float dx = x2 - x1;
		float a = (float)(time1*x2/x1 - time2).count() / (1 - exp(x2) - x2/x1 + exp(x1)*x2/x1);
		float b = (time2.count() - a * (1 - exp(x2))) / x2;

		cout << "estimated remaining/total time: " << timeof(a * (1 - exp(1)) + b - time2.count()) << " / " << timeof(a * (1 - exp(1)) + b)
			<< " pending samples: " << pendingsamples
			<< "  \r" << flush;
		if (timeoverflow)
		{
			//cout << endl << "estimator debug: a = " << a << " b = " << b << " x1 = " << x1 << " x2 = " << x2 << " time1 = " << time1.count() << " time2 = " << time2.count();
			//cout << flush << "\e[A\r";
		}
	}
	cout << endl;
	return nullptr;
}

void *imagep, *hdrimagep;

volatile bool last_event = false;
void*event_handler(void*)
{
	while (running)
	{
		usleep(100000/6);	if (!running)	break;
		last_event = handle_events(scene, xsystem, imagep);
	}
	return nullptr;
}

struct Task
{
	std::atomic_int count;	bool assigned=false;
};
void*samplep=nullptr;
void*taskp=nullptr;

float calculate_entropy(vector<vector3> samples)
{
	vector3 mean={0,0,0};
	for (auto&sample:samples)
		mean += sample;
	mean = mean / samples.size();
	float entropy=0;
	for (auto&sample:samples)
		entropy += norm(sample - mean);
	entropy /= samples.size()*samples.size();
	return entropy;
}

void*task_creator(void*)
{
	int xres = scene.current_camera->image_resolution[0], yres = scene.current_camera->image_resolution[1];
	auto tasks = (Task(*)[yres])taskp;
	auto samples = (vector<vector3>(*)[yres])samplep;
	//tasks.reserve(xresolution * yresolution);
	for (unsigned int y = 0; y < scene.current_camera->image_resolution[1]; y++)
		for (unsigned int x = 0; x < scene.current_camera->image_resolution[0]; x++)
			tasks[x][y].count=2, tasks[x][y].assigned=false;
	pendingsamples = xres * yres * (scene.current_camera->sample_count-2);
	return nullptr;

	while (pendingsamples > 0)
	{
		usleep(30000);
		struct Block {int x=0,y=0; float entropy=-INFINITY;}  max_block;
		int window_size = 100;
		float entropies[xres][yres];
		float mean_entropy=0;
		for (int y=0; y<yres; y++)
			for (int x=0; x<xres; x++) if (!tasks[x][y].assigned)
				mean_entropy += entropies[x][y] = calculate_entropy(samples[x][y]);
		mean_entropy /= xres*yres;
		for (int y=0; y<yres; y++)
			for (int x=0; x<xres; x++)
				if (!tasks[x][y].assigned && entropies[x][y] > mean_entropy)
					tasks[x][y].count++,
					pendingsamples--;
	}
	return nullptr;
}

void*viewer(void*)
{
	while (running)
	{
		usleep(40000);	if (!running)	break;
		XPutImage(display, window, gc, xsystem.image, 0, 0, 0, 0, scene.current_camera->image_resolution[0], scene.current_camera->image_resolution[1]);
	}
	XPutImage(display, window, gc, xsystem.image, 0, 0, 0, 0, scene.current_camera->image_resolution[0], scene.current_camera->image_resolution[1]);
	return nullptr;
}

vector3 sample(int x, int y)
{
	auto&camera = scene.current_camera;
	auto&right = xsystem.right;
	auto&up = xsystem.up;
	auto&deep = xsystem.deep;
	auto&screen_center = xsystem.screen_center;

	int xres = scene.current_camera->image_resolution[0], yres = scene.current_camera->image_resolution[1];
	double random_x = drand48()-0.5, random_y = drand48()-0.5;

	float ystep = (-camera->near_plane[2] + camera->near_plane[3]) / yres;
	float xstep = (-camera->near_plane[0] + camera->near_plane[1]) / xres;

	vector3 sampled_pixel_position = xsystem.screen_center + xsystem.right * ((float)x - xres/2 + random_x) * ystep + -xsystem.up * ((float)y - yres/2 + random_y) * xstep;

	vector3 sampled_camera_position = camera->position;
	vector3 target = (sampled_pixel_position +- camera->position) * (1 + camera->focus_distance / camera->near_distance);

	Ray ray = {sampled_camera_position, target};
	vector3 sampled_color = sendray(scene, ray, false,0,0, {x,y,true});
	//cout << "sampled position: " << sampled_pixel_position << endl;
	//cout << "sampled_color: " << sampled_color << endl;
	//exit(0);
	return sampled_color;
}

const int block_size = 10;
void sampler(int blockx, int blocky)
{
	int xres = scene.current_camera->image_resolution[0], yres = scene.current_camera->image_resolution[1];
	auto tasks = (Task(*)[yres])taskp;
	auto samples = (vector<vector3>(*)[yres])samplep;
	auto image = (vector3(*)[yres])imagep;
	//auto hdrimage = (vector3(*)[yres])hdrimagep;

	//cout << "sampler: " << blockx << " " << blocky << endl;

	for (int y=blocky; y<yres && y < blocky+block_size; y++)
	{
		for (int x=blockx; x<xres && x < blockx+block_size; x++)
		{
			for (int sampleindex=0; sampleindex<tasks[x][y].count; sampleindex++)
			{
				samples[x][y].push_back(sample(x, y));
				tasks[x][y].count--;
				//cout << "sampler: " << x << " " << y << " " << sampleindex << endl;
				totalsamples++;
			}
			vector3 color={0,0,0};
			for (auto&sample:samples[x][y])
				color += sample;
			color = color / samples[x][y].size();
			image[x][y] = clamp(color);
			xsystem.imagedata[y*xres+x] = (int)image[x][y].x << 16 | (int)image[x][y].y << 8 | (int)image[x][y].z;
		}
	}

	for (int y=blocky; y<yres && y < blocky+block_size; y++)
		for (int x=blockx; x<xres && x < blockx+block_size; x++)
			tasks[x][y].assigned = false;
};

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

		int xres = camera.image_resolution[0], yres = camera.image_resolution[1];

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
		Task tasks[xresolution][yresolution];	taskp = tasks;
		vector<vector3> samples[xresolution][yresolution];	samplep = samples;

		cout << "--- Rendering Started : " << camera.image_name << " ---" << endl;
		
		running = true;
		max_count = yresolution*xresolution*camera.sample_count;
		t0 = std::chrono::high_resolution_clock::now(), t1=t0;

		pthread_t estimating_thread;	pthread_create(&estimating_thread, nullptr, estimate, nullptr);	pthread_detach(estimating_thread);
		pthread_t event_thread;	pthread_create(&event_thread, nullptr, event_handler, nullptr);	pthread_detach(event_thread);
		//pthread_t task_thread;	pthread_create(&task_thread, nullptr, task_creator, nullptr);	pthread_detach(task_thread);
		pthread_t viewer_thread;	pthread_create(&viewer_thread, nullptr, viewer, nullptr);	pthread_detach(viewer_thread);

		float multiplier=1;	if (camera.pathtracing)	multiplier = 2 * M_PI;

			//int xres = scene.current_camera->image_resolution[0], yres = scene.current_camera->image_resolution[1];
			//auto tasks = (Task(*)[yres])taskp;
			//auto samples = (vector<vector3>(*)[yres])samplep;
			//tasks.reserve(xresolution * yresolution);
			for (unsigned int y = 0; y < scene.current_camera->image_resolution[1]; y++)
				for (unsigned int x = 0; x < scene.current_camera->image_resolution[0]; x++)
					tasks[x][y].count=2, tasks[x][y].assigned=false;
			pendingsamples = xres * yres * (scene.current_camera->sample_count-2);

		totalsamples=0;
		while (totalsamples < camera.sample_count*xresolution*yresolution)
		{
			//cout << "totalsamples: " << (int)totalsamples << " pendingsamples: " << pendingsamples << " " << endl;

			#pragma omp parallel for
			for (int y=0; y<yresolution; y++)
				for (int x=0; x<xresolution; x++)
				{
					if (!tasks[x][y].assigned && tasks[x][y].count)
					{
						for (int bx=x; bx<x+block_size && bx<xresolution; bx++)
							for (int by=y; by<y+block_size && by<yresolution; by++)
								tasks[bx][by].assigned = true;
						sampler(x, y);
					}
				}

			struct Block {int x=0,y=0; float entropy=-INFINITY;}  max_block;
			int window_size = 100;
			float entropies[xres][yres];
			float mean_entropy=0;
			for (int y=0; y<yres; y++)
				for (int x=0; x<xres; x++) if (!tasks[x][y].assigned)
					mean_entropy += entropies[x][y] = calculate_entropy(samples[x][y]);
			mean_entropy /= xres*yres;
			for (int y=0; y<yres; y++)
				for (int x=0; x<xres; x++)
					if (!tasks[x][y].assigned && entropies[x][y] > mean_entropy)
						tasks[x][y].count++,
						pendingsamples--;
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