#pragma once
#include "vector.hh"
#include "objects.hh"
#include <iostream>
struct Intersection
{
	bool intersecting=false;
	vector3 position;
	vector3 normal;
	enum {triangle,sphere,box,mesh} object;
	Sphere*sphere_p=nullptr;
	Triangle*triangle_p=nullptr;
	AlignedBox*box_p=nullptr;
	Mesh*mesh_p=nullptr;
	Object* getobject()
	{
		switch (object)
		{
		case triangle:
			return triangle_p;
		case sphere:
			return sphere_p;
		case box:
			return box_p;
		case mesh:
			return mesh_p;
		}
		return nullptr;
	}
	vector<Texture> gettextures()
	{
		switch (object)
		{
		case mesh:
			return mesh_p->gettextures();
		case sphere:
			return sphere_p->gettextures();
		case triangle:	//no textures for unmeshed triangles
			return vector<Texture>();
		default:
			std::cout << "intersection::gettextures() was called on an object that cannot have textures" << std::endl;
			exit(1);
		}
	}
	vector3 get2dcoordinates()
	{
		switch (object)
		{
		case triangle:
		case mesh:
			if (triangle_p == nullptr)
			{
				std::cout << "triangle_p is nullptr in intersection" << std::endl;
				exit(1);
			}
			if (mesh_p == nullptr)
			{
				std::cout << "mesh_p is nullptr in intersection" << std::endl;
				exit(1);
			}
			return triangle_p->get2dcoordinates(mesh_p->inverse * position);
		case sphere:
			return sphere_p->get2dcoordinates(position);
		default:
			std::cout << "intersection::get2dcoordinates() was called on an object that cannot have 2d coordinates" << std::endl;
			exit(1);
		}
	}
};