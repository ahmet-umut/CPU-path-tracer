#include "intersection.hh"
#include "Ray.hh"
#include "vector_utilities.hh"
#include <iostream>
using namespace std;
struct Intersection intersect(Scene&scene, Ray ray, bool verbose)
{
	struct Intersection closest_intersection;
	float distance = INFINITY;
	for (auto&triangle : scene.triangles)
	{
		struct Intersection intersection = ray.intersect(triangle);
		if (intersection.intersecting)	if (norm(intersection.position +- ray.start) < distance)	
		{
			intersection.object = intersection.triangle;
			distance = norm(intersection.position +- ray.start);
			closest_intersection = intersection;
		}
	}
	for (auto&sphere : scene.spheres)
	{
		struct Intersection intersection = ray.intersect(sphere);
		if (intersection.intersecting)	if (norm(intersection.position +- ray.start) < distance)	
		{
			intersection.object = intersection.sphere;
			intersection.sphere_p = &sphere;
			distance = norm(intersection.position +- ray.start);
			closest_intersection = intersection;
		}
	}
	for (auto&mesh : scene.meshes)
	{
		struct Intersection intersection = ray.intersect(mesh, scene.debug_level);
		if (intersection.intersecting)	if (norm(intersection.position +- ray.start) < distance)	
		{
			intersection.object = intersection.mesh;
			intersection.mesh_p = &mesh;
			distance = norm(intersection.position +- ray.start);
			closest_intersection = intersection;
		}
	}
/* 	struct intersection intersection = ray.intersect(scene.gigabox);
	if (intersection.intersecting)	if (norm(intersection.position +- ray.start) < distance)	
	{
		if (intersection.intersecting)	if (norm(intersection.position +- ray.start) < distance)	
			{
				distance = norm(intersection.position +- ray.start);
				closest_intersection = intersection;
			}
	} */
	return closest_intersection;
}