#include "test.hh"
#include "vector_utilities.hh"

vector3 test(Xsystem&xsystem, const vector3&vertex)
{
	float distance = norm(vertex +- xsystem.camera.position);
	float distance2 = dot(xsystem.deep, vertex +- xsystem.screen_center);
	vector3 projection = vertex +- xsystem.deep * distance2;
	float u = dot(xsystem.right, vertex +- xsystem.screen_center), v = dot(xsystem.up, vertex +- xsystem.screen_center);
	//v = 1 - v;
	u*= xsystem.camera.near_distance / (xsystem.camera.near_distance + distance2) / xsystem.xstep;
	v*= xsystem.camera.near_distance / (xsystem.camera.near_distance + distance2) / xsystem.ystep;
	//u+= xresolution/2;
	//v+= yresolution/2;
	u += xsystem.camera.image_resolution[0]/2;
	v += xsystem.camera.image_resolution[1]/2;
	v = xsystem.camera.image_resolution[1] - v;
	return {u,v,0};
}