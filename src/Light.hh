#pragma once
#include "vector.hh"
#include "Ray.hh"
class Light
{
public:
	virtual Ray sample_ray(const vector3 & start) const = 0;
	virtual vector3 getradiance(Ray & ray) const = 0;
};

class _PointLight : public Light
{
public:
	vector3 position, intensity;
	_PointLight(vector3 position, vector3 intensity) : position(position), intensity(intensity) {}
	Ray sample_ray(const vector3 & start) const override
	{
		Ray ray(start, position-start);
		ray.length = norm(position - start);
		return ray;
	}
	vector3 getradiance(Ray & ray) const override
	{
		return intensity / powf(ray.length, 2);
	}
};

class _AreaLight : public Light
{
public:
	vector3 position, normal;	float size;
	vector3 radiance;
	_AreaLight(vector3 position, vector3 normal, float size, vector3 radiance) : position(position), normal(normal), size(size), radiance(radiance) {}
	Ray sample_ray(const vector3 & start) const override
	{
		auto perp_dir = [](const vector3& vec) -> vector3
		{
			vector3 result(vec.y, -vec.x, 0);
			if (vec.x == 0 && vec.y == 0) result.x = 1;
			return result;
		};
		vector3 u = perp_dir(normal), v = normal.cross(u);
		vector3 sampled = position + u * size * (drand48() - .5) + v * size * (drand48() - .5);
		Ray ray(start, sampled-start);
		ray.length = norm(sampled - start);
		return ray;
	}
	vector3 getradiance(Ray & ray) const override
	{
		return radiance / powf(ray.length, 2) * powf(size, 2) * abs(dot(normal, normalized(ray.getdirection())));
	}
};

class _DirectionalLight : public Light
{
public:
	float xzangl, yangl;
	vector3 radiance;
	_DirectionalLight(float xzangl, float yangl, vector3 radiance) : xzangl(xzangl), yangl(yangl), radiance(radiance) {}
	Ray sample_ray(const vector3 & start) const override
	{
		return Ray(start, xzangl, yangl);
	}
	vector3 getradiance(Ray & ray) const override
	{
		return radiance;
	}
};

class _SpotLight : public Light
{
public:
	vector3 position, direction;
	float coverage_angle, falloff_angle;
	vector3 intensity;
	_SpotLight(vector3 position, vector3 direction, float coverage_angle, float falloff_angle, vector3 intensity) : position(position), direction(direction), coverage_angle(coverage_angle), falloff_angle(falloff_angle), intensity(intensity) {}
	Ray sample_ray(const vector3 & start) const override
	{
		Ray ray(start, position-start);
		ray.length = norm(position - start);
		return ray;
	}
	vector3 getradiance(Ray & ray) const override
	{
		float cos_spot_angle = cos_vv(direction, -ray.getdirection());
		if (cos_spot_angle < cos(coverage_angle))
			return {0,0,0};
		float multiplier = 1;
		if (cos_spot_angle < cos(falloff_angle))
			multiplier = pow((cos_spot_angle - cos(coverage_angle)) / (cos(falloff_angle) - cos(coverage_angle)), 4);
		return intensity / powf(ray.length, 2) * multiplier;
	}
};