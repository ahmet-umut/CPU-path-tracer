#pragma once
#include "vector.hh"
#include "objects.hh"
#include "intersection.hh"
class Ray
{
public:
	vector3 start, direction;
	float xzangl=INFINITY, yangl=INFINITY;
	float length;

	Ray() : length(-1) {}
	Ray(const vector3&start) : start(start), length(-2) {}
	Ray(const vector3&start, const vector3&direction, float length=INFINITY);
	Ray(const vector3&start, const float & xzangl, const float & yangl);
	
	struct Intersection intersect(Triangle& triangle);
	struct Intersection intersect(Sphere& sphere, bool transformed=false);
	struct Intersection intersect(AlignedBox& box, bool verbose = false, int debug = -1, int depth = 0);
	struct Intersection intersect(Mesh& mesh, int debug_level=-1);	//, bool triangle_debug = false);

	vector3 getend();
	vector3&getdirection();
	float getlength() const;
	float getxzangl();
	float getyangl();
};