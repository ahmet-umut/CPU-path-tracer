#include "sphere2d.hh"
#include "Ray.hh"
vector3 sphere2dlatlong(vector3 point, vector3 center, float radius, bool flipv)
{
	Ray direction = {center,point-center};
	float u,v;
	float xz=direction.getxzangl();
	float y =direction.getyangl();
	//u = fmod(xz/M_PI, 2)/2;
	u = fmod(fmod(	-xz/M_PI, 2)+2+1.5, 2) / 2;
	v = fmod(fmod((y+M_PI_2)/M_PI, 1)+1, 1); //to make it positive
	if (flipv)
		v = 1-v;
	return {u, v, 0};
}
vector3 sphere2dprobe(vector3 point, vector3 center, float radius)
{
	Ray direction = {center,point-center};
	float u,v;
	float xz=direction.getxzangl();
	float y =direction.getyangl();
	float modxz = fmod(fmod(xz, 2*M_PI)+2*M_PI, 2*M_PI);
	float sy = sin(y), cy = cos(y);
	u = cy + modxz / 2 / M_PI * (1 - 2 * cy);
	v = sy/2+0.5;
	return {u, v, 0};
}