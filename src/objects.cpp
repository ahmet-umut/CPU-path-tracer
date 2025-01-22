#include "objects.hh"
static inline void mv(const matrix& m, const vector4& v, vector4& result)
{
	cblas_sgemv(CblasRowMajor, CblasNoTrans, 4, 4, 1, (float*)&m, 4, (float*)&v, 1, 0, (float*)&result, 1);
}
void Triangle::transform(const matrix& m)
{
	vector4 homogen = vertex1;
	homogen.x3 = 1;
	vertex1 = m * homogen;
	homogen = vertex2delta;
	homogen.x3 = 0;
	vertex2delta = m * homogen;
	homogen = vertex3delta;
	homogen.x3 = 0;
	vertex3delta = m * homogen;
	normal_calculated = false;
}
#include <iostream>
using namespace std;
void Sphere::transform(const matrix& m)
{
	cout << "transforming sphere" << endl;

	vector4 homogen = center;
	homogen.x3 = 1;
	center = m * homogen;
}
void AlignedBox::transform(const matrix& m)
{
	vector4 homogen = min;
	homogen.x3 = 1;
	min = m * homogen;
	homogen = max;
	max = m * homogen;
}

#include "contains.hh"
#include "math.h"	// fmin, fmax, ...
static void extend(vector3&min, vector3&max, const vector3 point)
{
	if (!contains(min, max, point))
		min.x = fmin(min.x, point.x),
		min.y = fmin(min.y, point.y),
		min.z = fmin(min.z, point.z),
		max.x = fmax(max.x, point.x),
		max.y = fmax(max.y, point.y),
		max.z = fmax(max.z, point.z);
}

void AlignedBox::embrace(Triangle& triangle, uint index)
{
	
	extend(min, max, triangle.vertex1);
	extend(min, max, triangle.vertex1 + triangle.vertex2delta);
	extend(min, max, triangle.vertex1 + triangle.vertex3delta);
	start_indice = std::min(start_indice, index);
	end_indice = std::max(end_indice, index+1);
}
void AlignedBox::embrace(AlignedBox*box)
{
	extend(min, max, box->min);
	extend(min, max, box->max);
	children.push_back(box);
}
void AlignedBox::embrace(Triangle*triangle)
{
	extend(min, max, triangle->vertex1);
	extend(min, max, triangle->vertex1 + triangle->vertex2delta);
	extend(min, max, triangle->vertex1 + triangle->vertex3delta);
	triangles.push_back(*triangle);
}

bool AlignedBox::isEmpty() { return isinf(min.x) || isinf(max.x); }
