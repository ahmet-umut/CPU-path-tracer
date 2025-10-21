#include "Ray.hh"
#include "vector_utilities.hh"
#include <iostream>
using namespace std;

Ray::Ray(const vector3&start, const vector3&direction, float length):
	start(start),
	direction(direction),
	//xzangl(atan2(direction.z, direction.x)),
	//yangl(atan2(direction.y, sqrt(direction.x*direction.x + direction.z*direction.z))),
	length(length)
{}
Ray::Ray(const vector3&start, const float & xzangl, const float & yangl):
	start(start),
	xzangl(xzangl),
	yangl(yangl),
	length(-4)
{}

float Ray::getxzangl()
{
	if (xzangl!=INFINITY)
		return xzangl;
	//cout << "initializing xzangl" << endl;
	return xzangl = atan2(direction.z, direction.x);
}
float Ray::getyangl()
{
	if (yangl!=INFINITY)
		return yangl;
	//cout << "initializing yangl" << endl;
	return yangl = atan2(direction.y, sqrt(direction.x*direction.x + direction.z*direction.z));
}
vector3 Ray::getend()
{
	if (length==-1)
	{
		cout << "Ray::getend() was called on a uninitialized ray: -1" << endl;
		exit(1);
	}
	else if (length==-2)
	{
		cout << "Ray::getend() was called on a partially initialized ray: -2" << endl;
		exit(1);
	}
	else if (length==-4)
	{
		cout << "Ray::getend() was called on a infinite ray: -4" << endl;
		exit(1);
	}
	else return start + getdirection().normalize() * length;
}
vector3& Ray::getdirection()
{
	if (length==-1)	//direction cannot be determined
	{
		cout << "Ray::getdirection() was called on a uninitialized ray: -1" << endl;
		exit(1);
	}
	else if (length==-2)	//direction cannot be determined
	{
		cout << "Ray::getdirection() was called on a partially initialized ray: -2" << endl;
		exit(1);
	}
	else if (length==-4)	//direction vector can be determined but has not been yet
	{
		length = INFINITY;
		return direction = vector3(xzangl, yangl);
	}
	else return direction;
}
float Ray::getlength() const
{
	if (length==-1)
	{
		cout << "Ray::getlength() was called on a uninitialized ray: -1" << endl;
		exit(1);
	}
	else if (length==-2)
	{
		cout << "Ray::getlength() was called on a partially initialized ray: -2" << endl;
		exit(1);
	}
	else if (length==-4)
	{
		return INFINITY;
	}
	else return length;
}


struct Intersection Ray::intersect(Triangle& triangle)
{
	using namespace std;
	const vector3 & r0 = start;
	const vector3 & v0 = triangle.vertex1;
	vector3 &e1 = triangle.vertex2delta, &e2 = triangle.vertex3delta;

	float epsilon = .1e-8f;

	//float cos_yangl = cos(yangl);
	//vector3 d = {cos(xzangl) * cos_yangl, sin(yangl), sin(xzangl) * cos_yangl};
	vector3 d = getdirection().normalize();

	vector3 pvec = d.cross(e2);
	float det = cblas_sdot(3, &e1, 1, &pvec, 1);

	if (fabs(det) < epsilon)
		return {false}; // No intersection, ray is parallel to the triangle

	float inv_det = 1.0f / det;

	vector3 tvec = r0 +- v0;
	//float u = vectorDotProduct(tvec, pvec) * inv_det;
	float u = cblas_sdot(3, &tvec, 1, &pvec, 1) * inv_det;
	if (u < 0 || u > 1)
		return {false}; // No intersection

	vector3 qvec = tvec.cross(e1);
	//float v = vectorDotProduct(d, qvec) * inv_det;
	float v = cblas_sdot(3, &d, 1, &qvec, 1) * inv_det;
	if (v < 0 || u + v > 1)
		return {false}; // No intersection

	//float t = vectorDotProduct(e2, qvec) * inv_det;
	float t = cblas_sdot(3, &e2, 1, &qvec, 1) * inv_det;
	if (t < 0)
		return {false}; // Intersection behind the ray start

	if (t > getlength())
		return {false}; // Intersection beyond the segment length

	vector3 intersection_position = r0 + d * t;
	return {true, intersection_position, triangle.normal(), Intersection::triangle, nullptr, &triangle};
}
static vector3 transform(const matrix& m, vector3 v, bool position=true)
{
	if (m.is_identity())	return v;
	vector3 result3;
	vector4 result4;
	switch (position)
	{
	case false:
		cblas_sgemv(CblasRowMajor, CblasNoTrans, 3, 3, 1, (float*)&m, 4, (float*)&v, 1, 0, (float*)&result3, 1);	//lda is 4 because m is a 4x4 matrix
		return result3;
	case true:
		result4 = v;	result4.x3=1;
		result4 *= m;
		return result4;	
	}
}
using namespace std;
#include <iostream>
struct Intersection Ray::intersect(Sphere& sphere, bool transformed)
{
	if (!transformed)
	{
		Ray ray = {transform(sphere.inverse, start), transform(sphere.inverse, getdirection(), false)};
		//ray = *this;

		struct Intersection intersection = ray.intersect(sphere,true);
		if (intersection.intersecting)
		{
			intersection.position = transform(sphere.transformation, intersection.position);
			float t = norm(intersection.position - ray.start);
			if (t > getlength())
				return {false}; // Intersection beyond the segment length
			matrix transpose = sphere.inverse;
			mkl_simatcopy('R', 'T', 4, 4, 1, (float*)&transpose, 4,4);	// Transpose the inverse matrix
			intersection.normal = transform(transpose, intersection.normal, false);
			return intersection;
		}
		return {false};
	}

	vector3 l = sphere.center - start;
	vector3 d = getdirection().normalize();
	float tca = cblas_sdot(3, &l, 1, &d, 1);
	
	if (tca < 0)
		return {false}; // No intersection

	//float d2 = l.dot(l) - tca * tca;
	float d2 = cblas_sdot(3, &l, 1, &l, 1) - tca * tca;
	float radius2 = sphere.radius * sphere.radius;
	if (d2 > radius2)
		return {false}; // No intersection

	float thc = sqrt(radius2 - d2);
	float t0 = tca - thc;
	float t1 = tca + thc;

	if (t0 > getlength() || t1 < 0)
		return {false}; // No intersection

	float t = t0;
	if (t0 < 0)
		t = t1;

	//vector3 intersection_position = start + vector3(cos(xzangl) * cos(yangl), sin(yangl), sin(xzangl) * cos(yangl))*t;
	vector3 intersection_position = start + d * t;
	vector3 normal = (intersection_position +- sphere.center);
	normal.normalize();
	return {true, intersection_position, normal};
}
#include <mkl_lapacke.h>
#include <iostream>
using namespace std;
struct Intersection Ray::intersect(Mesh& mesh, int debug_level)
{
	Ray ray;
	ray = {transform(mesh.inverse, start), transform(mesh.inverse, getdirection(), false)};
	//ray = *this;

	struct Intersection intersection = ray.intersect(*mesh.data, false, debug_level);
	if (intersection.intersecting)
	{
		intersection.position = transform(mesh.transformation, intersection.position);
		float t = norm(intersection.position - ray.start);
		if (t > getlength())
			return {false}; // Intersection beyond the segment length
		//mkl_simatcopy('R', 'T', 4, 4, 1, (float*)&mesh.inverse, 4,4);	// Transpose the mesh.inverse matrix
		intersection.normal = transform(mesh.transpose, intersection.normal, false);
		return intersection;
	}
	return {false};
}
#include <iostream>
using namespace std;
struct Intersection Ray::intersect(AlignedBox& box, bool verbose, int debug, int depth)
{
	float&x1 = box.min.x;
	float&x2 = box.max.x;
	float&y1 = box.min.y;
	float&y2 = box.max.y;
	float&z1 = box.min.z;
	float&z2 = box.max.z;

	/* if (x1 != x1)	{cout << "x1 is nan" << endl; exit(1);}
	if (x2 != x2)	{cout << "x2 is nan" << endl; exit(1);}
	if (y1 != y1)	{cout << "y1 is nan" << endl; exit(1);}
	if (y2 != y2)	{cout << "y2 is nan" << endl; exit(1);}
	if (z1 != z1)	{cout << "z1 is nan" << endl; exit(1);}
	if (z2 != z2)	{cout << "z2 is nan" << endl; exit(1);} */

	if (start.x != start.x)	{cout << "start.x is nan" << endl; exit(1);}
	if (start.y != start.y)	{cout << "start.y is nan" << endl; exit(1);}
	if (start.z != start.z)	{cout << "start.z is nan" << endl; exit(1);}

/*	vector3 second(xzangl, yangl);
	second += start;
 	if (second.x != second.x)	{cout << "second.x is nan" << endl; exit(1);}
	if (second.y != second.y)	{cout << "second.y is nan" << endl; exit(1);}
	if (second.z != second.z)	{cout << "second.z is nan" << endl; exit(1);}
	float divx = second.x-start.x;	//if (divx == 0)	divx = 1e-8;
	float divy = second.y-start.y;	//if (divy == 0)	divy = 1e-8;
	float divz = second.z-start.z;	//if (divz == 0)	divz = 1e-8;
 */

	vector3 dir = getdirection().normalize();
	float t1 = (x1 - start.x) / dir.x;
	float t2 = (x2 - start.x) / dir.x;
	float t3 = (y1 - start.y) / dir.y;
	float t4 = (y2 - start.y) / dir.y;
	float t5 = (z1 - start.z) / dir.z;
	float t6 = (z2 - start.z) / dir.z;
	
	#define handle(t) {/* cout << "tx is nan" << endl; */ return {false};}
	if (t1 != t1)	handle(t1);
	if (t2 != t2)	handle(t2);
	if (t3 != t3)	handle(t3);
	if (t4 != t4)	handle(t4);
	if (t5 != t5)	handle(t5);
	if (t6 != t6)	handle(t6);
	#undef handle

	float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
	float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));
	
	//cout << tmin << " " << tmax << endl;

	if (tmin > tmax || tmax < 0)	return {false};
	//if (tmin-tmax > 1e-4 || tmax < -1e-4)	return {false};

	if (verbose)	cout << "ray hit " << box.say() << endl;
	
	if (debug!=depth)
	{
		struct Intersection closest_intersection;
		float closest_distance = INFINITY;
		for (auto&child: box.children)
		{
			struct Intersection intersection = intersect(*child, verbose, debug, depth+1);
			if (intersection.intersecting)
			{
				float distance = norm(intersection.position +- start);
				if (distance < closest_distance)	
				{
					closest_distance = distance;
					closest_intersection = intersection;
				}
			}
		}
		for (auto&mesh : box.meshes)
		{
			struct Intersection intersection = intersect(*(Mesh*)mesh);
			if (intersection.intersecting)	if (norm(intersection.position +- start) < closest_distance)	
			{
				closest_distance = norm(intersection.position +- start);
				closest_intersection = intersection;
			}
		}
		for (auto&triangle: box.triangles)
		{
			struct Intersection intersection = intersect(triangle);
			if (intersection.intersecting)
			{
				float distance = norm(intersection.position +- start);
				if (distance < closest_distance)	
				{
					closest_distance = distance;
					closest_intersection = intersection;
					intersection.triangle_p = &triangle;
				}
			}
		}
		return closest_intersection;
	}
	else
	{
		float t_intersection = tmin < 0 ? tmax : tmin;
		vector3 normal;
		if (t_intersection == t1)	normal = {-1,0,0};
		else if (t_intersection == t2)	normal = {1,0,0};
		else if (t_intersection == t3)	normal = {0,-1,0};
		else if (t_intersection == t4)	normal = {0,1,0};
		else if (t_intersection == t5)	normal = {0,0,-1};
		else if (t_intersection == t6)	normal = {0,0,1};
		else
		{
			cout << "normal not found in box intersection" << endl;
			/* cout << t_intersection << endl;
			cout << t1 << " " << t2 << " " << t3 << " " << t4 << " " << t5 << " " << t6 << endl;
			cout << start << " " << second << endl; */
			exit(1);
		}
		return {true, start + dir * tmin, normal, Intersection::box, nullptr, nullptr, &box};
	}
}