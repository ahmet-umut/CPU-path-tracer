#pragma once
#include <memory.h>
#include <mkl_cblas.h>
#include <libxml2/libxml/parser.h>
#include <math.h>
#include <ostream>
#include <mkl.h>

struct vector3;
struct vector4;
struct matrix;

struct vector4
{
	float x,y,z,x3;
	float*operator&() {return (float*)this;}
	vector4() {}
	vector4(const vector3& v) {memcpy(this, &v, 3*4/*3 floats*/);}
	vector4(float x, float y, float z, float x3) : x(x), y(y), z(z), x3(x3) {}
	vector4(float x, float y, float z) : x(x), y(y), z(z) {}
	float&operator[](const int i) const {return ((float*)this)[i];}
	void operator*= (const matrix&m);
	bool operator==(const vector4& v) const	{return x == v.x && y == v.y && z == v.z && x3 == v.x3;}
};
struct vector3
{
	float x,y,z;
	float*operator&() {return (float*)this;}
	vector3() {}
	vector3(const vector4& v) {memcpy(this, &v, 3*4/*3 floats*/);}
	vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	vector3(float xzangl, float yangl) : x(cos(xzangl) * cos(yangl)), y(sin(yangl)), z(sin(xzangl) * cos(yangl)) {}
	vector3(auto container) : x(container[0]), y(container[1]), z(container[2]) {}
	vector3 operator+(const vector3& v) const;
	void operator+=(const vector3& v);
	vector3 operator*(float f) const;
	vector3 operator/(float f) const {return *this * (1/f);}
	vector3 operator-() const {return *this * -1;}
	vector3 operator-(const vector3& v) const;
	bool operator==(const vector3& v) const	{return x == v.x && y == v.y && z == v.z;}

	vector3 cross(const vector3& v) const;
	vector3 apply(auto f) const {return {f(x), f(y), f(z)};}
	vector3 elementwise(const vector3& v) const {return {x*v.x, y*v.y, z*v.z};}
	vector3& normalize();
	vector3 getnormalized() const;
};
struct matrix
{
	vector4 row0, row1, row2, row3;
	float*operator&() {return (float*)this;}
	vector4 operator*(const vector4& v) const;
	matrix operator*(const matrix& m) const;
	vector4&operator[](const int i) const;
	static matrix identity();
	matrix() {}
	//matrix(xmlNode*node, Tempor tempor);
	matrix(const matrix&m) {memcpy(this, &m, 4*4*sizeof(float));}

	void invert();
	void transpose();
	bool is_identity() const;
};
