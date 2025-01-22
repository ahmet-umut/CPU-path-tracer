#pragma once
#include "vector.hh"
#include <libxml2/libxml/parser.h>
struct Material
{
	float phong_exponent=0;
	vector3 ambient, diffuse, specular = {0,0,0}, mirror = {0, 0, 0};
	vector3 AbsorptionCoefficient = {0, 0, 0};
	float refraction_index;

	Material(){}
	Material(xmlNode*node);

	bool is_mirror()
	{
		return mirror.x > 0 || mirror.y > 0 || mirror.z > 0;
	}
	bool is_dielectric()
	{
		return AbsorptionCoefficient.x > 0 || AbsorptionCoefficient.y > 0 || AbsorptionCoefficient.z > 0;
	}
};