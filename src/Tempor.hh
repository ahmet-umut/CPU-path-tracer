#pragma once
#include <vector>
#include "Material.hh"
#include "Texture.hh"
struct Tempor
{
	std::vector<Material> materials;
	std::vector<vector3> vertices;
	std::vector<Texture> textures;
	std::vector<matrix> transformations[4];
	void add_transformation(xmlNode*node);
	std::vector<int> texture_indices;
};