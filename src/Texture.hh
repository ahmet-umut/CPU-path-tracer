#pragma once
#include <libxml2/libxml/parser.h>
#include <vector>
#include "vector.hh"
struct Texture
{
	enum {null, bump,all,kd_replace,kd_blend,background} type;
	enum {image,kareli,noise} source=image;
	enum {bilinear} interpolation;
	enum {absval,linear} noise_conversion;
	int image_index;
	float normalizer;
	Texture(xmlNode*node);
	vector3 getcolor(vector3 coords, std::vector<std::vector<std::vector<vector3>>> &images);
};