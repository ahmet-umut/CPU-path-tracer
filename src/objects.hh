#pragma once
#include <vector>
#include <mkl.h>
#include <mkl_blas.h>
#include <string>
#include <math.h>

#include "Material.hh"
#include "Texture.hh"
#include "Tempor.hh"

using std::vector, std::string;
class Object
{
public:
    virtual Material&getmaterial() = 0;
	virtual vector<Texture> gettextures() = 0;
    virtual string say() = 0;
    virtual uint getid() = 0;
	virtual void transform(const matrix& m) = 0;
	virtual vector3 get2dcoordinates(vector3 point) { return {0,0,0}; }
	virtual bool islightsource() { return false; }
	virtual vector3 getradiance() { return {0,0,0}; }
};

class Triangle : public Object
{
public:
    vector3 vertex1, vertex2delta, vertex3delta;
    Material material;
    //vector3 normal;
    uint objectid;
	bool normal_calculated = false;

	bool texture_mapped=false;
	float u1, u2, u3;	float v1, v2, v3;

	Triangle(){}
	Triangle(uint objectid, vector3 vertex1, vector3 vertex2, vector3 vertex3, Material material) : objectid(objectid), vertex1(vertex1), material(material)
	{
		vsSub(3, &vertex2, &vertex1, &vertex2delta);
		vsSub(3, &vertex3, &vertex1, &vertex3delta);
	}
	Triangle(xmlNode*node, Tempor&tempor);

    Material&getmaterial() { return material; }
	vector<Texture> gettextures() { return vector<Texture>(); }
    string say() { return "triangle"; }
    uint getid() { return objectid; }
	void transform(const matrix& m);
	vector3 normal();
	vector3 get2dcoordinates(vector3 point);
};
class Sphere : public Object
{
public:
    vector3 center;
    float radius;
    Material material;	vector<Texture> textures;
    int objectid;
	matrix transformation = matrix::identity(), inverse = matrix::identity();
	bool is_lightsource = false;	vector3 radiance;

    Sphere(uint objectid, vector3 center, float radius, Material material) : objectid(objectid), center(center), radius(radius), material(material) {}
	Sphere(xmlNode*node, Tempor&tempor);

    Material&getmaterial() { return material; }
	vector<Texture> gettextures() { return textures; }
    string say() { return "sphere"; }
    uint getid() { return objectid; }
	void transform(const matrix& m);
	vector3 get2dcoordinates(vector3 point);//Calculates the 2D coordinates of a point by "implicitly projecting" it onto the sphere
	bool islightsource() { return is_lightsource; }
	vector3 getradiance() { return radiance; }
};
class AlignedBox : public Object
{
public:
	//AlignedBox3f box, _box;
	//vector3 origin, dimens, origin_, dimens_;
	vector3 min, max, _min,_max;

	uint objectid;
	
	uint start_indice, end_indice;
	vector<Triangle> triangles;
	bool no_more = false;

	AlignedBox* parent=nullptr;
	vector<AlignedBox*> children;
	vector<Object*> meshes;

	AlignedBox() :
		min(INFINITY, INFINITY, INFINITY),
		max(-INFINITY, -INFINITY, -INFINITY)
	{}

	//AlignedBox(uint objectid, AlignedBox3f box, Material material) : objectid(objectid), box(box), material(material) {}
	//AlignedBox(uint objectid, vector3 origin, vector3 dimens, Material material) : objectid(objectid), origin(origin), dimens(dimens), material(material) {}
	AlignedBox(uint objectid, vector3 min, vector3 max, Material material) : objectid(objectid), min(min), max(max) {}

	Material&getmaterial()
	{
		static Material m;
		return m;
	}
	vector<Texture> gettextures() { return vector<Texture>(); }
	string say() { return "box"; }
	uint getid() { return objectid; }
	void transform(const matrix& m);
	
	void embrace(Triangle& triangle, uint index);
	void embrace(AlignedBox*box);
	void embrace(Triangle*triangle);
	bool isEmpty();
};
class Mesh : public Object
{
public:
	uint objectid;
	matrix transformation = matrix::identity(), inverse = matrix::identity(), transpose = matrix::identity();
	Material material;
	vector<Texture>textures;
	AlignedBox* data;
	bool is_lightsource = false;	vector3 radiance;
	vector3 center;

	Mesh(){}
	Mesh(xmlNode*node, Tempor&tempor, uint&count);	//mesh
	Mesh(xmlNode*node, Tempor&tempor, vector<Mesh>&meshes);	//mesh instance

	Material&getmaterial() { return material; }
	vector<Texture> gettextures() { return textures; }
	string say() { return "mesh"; }
	uint getid() { return objectid; }
	void transform(const matrix& m)
	{
		transformation = m * transformation;
	}
	bool islightsource() { return is_lightsource; }
	vector3 getradiance() { return radiance; }
};