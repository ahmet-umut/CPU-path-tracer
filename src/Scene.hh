#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <X11/Xlib.h>
#include <libxml2/libxml/parser.h>
#include "objects.hh"

using std::vector;
//using vector3=double[3];

struct Camera
{
	vector3 position, gaze, up;
	float near_plane[4], near_distance;
	uint16_t image_resolution[2];
	std::string image_name;
	unsigned char sample_count=1;
	float focus_distance=0, aperture_size=0;
	bool hdr=false, pathtracing=false, nee=false, importance_sampling=false;
	float key_value, burn_percent, saturation, gamma;
	int splitCount=1;
	Camera(xmlNode*node);
	Camera(){}
};
struct PointLight
{
	vector3 position;
	vector3 intensity;
	PointLight(xmlNode*node);
};
struct AreaLight
{
	vector3 position;
	vector3 radiance;
	vector3 normal;
	float size;    
	AreaLight(xmlNode*node);
};
struct Background
{
	int image_indice;
};
struct DirectionalLight
{
	float xzangl, yangl;
	vector3 radiance;
	DirectionalLight(xmlNode*node);
};
struct SpotLight
{
	vector3 position, direction, intensity;
	float coverage_angle, falloff_angle;
	SpotLight(xmlNode*node);
};

struct Scene
{
	vector3 background_color;
	vector3 ambient_light;
	int max_recursion_depth=1;
	float shadow_ray_epsilon=.001;
	vector<Camera> cameras;
	Camera*current_camera;
	vector<PointLight> point_lights;
	vector<AreaLight> area_lights;
	enum {color, latlong,probe,replace} background_type = color;
	Background background;
	vector<DirectionalLight> directional_lights;
	vector<SpotLight> spot_lights;
	vector<Sphere> spheres;
	vector<Triangle> triangles;
	vector<Mesh> meshes;
	AlignedBox gigabox;
	vector<vector<vector<vector3>>> images;
	uint triangle_count=0;
	int debug_level=-1;
	bool pathtracing=true;
	Scene(const char * filename);
	Scene(){}

	vector<vector3> vertices;	//debug
};
