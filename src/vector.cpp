#include "vector.hh"
#include <mkl.h>
//#include <mkl_pblas.h>

matrix matrix::operator*(const matrix& m) const
{
	matrix result;
	cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 4, 4, 4, 1, (float*)this, 4, (float*)&m, 4, 0, (float*)&result, 4);
	return result;
}
matrix matrix::identity()
{
	matrix m;
	m.row0 = {1,0,0,0};
	m.row1 = {0,1,0,0};
	m.row2 = {0,0,1,0};
	m.row3 = {0,0,0,1};
	return m;
}
vector3 vector3::cross(const vector3& v) const
{
	vector3 result;
	result.x = y*v.z - z*v.y;
	result.y = z*v.x - x*v.z;
	result.z = x*v.y - y*v.x;
	return result;
}
vector3 vector3::operator*(float f) const
{
	vector3 result=*this;
	//vsMul(3, (float*)this, &f, (float*)&result);
	cblas_sscal(3, f, (float*)&result, 1);
	return result;
}
vector4& matrix::operator[](const int i) const
{
	return *((vector4*)this + i);
}

float dot(const vector3&&v1, const vector3&&v2)
{
	return cblas_sdot(3, (float*)&v1, 1, (float*)&v2, 1);
}

vector3& vector3::normalize()
{
	cblas_sscal(3, 1.f / cblas_snrm2(3, (float*)this, 1), (float*)this, 1);
	return *this;
}
vector3 vector3::getnormalized() const
{
	vector3 result = *this;
	cblas_sscal(3, 1.f / cblas_snrm2(3, (float*)this, 1), (float*)&result, 1);
	return result;
}

void vector3::operator+=(const vector3& v)
{
	cblas_saxpy(3, 1, (float*)&v, 1, (float*)this, 1);
}

void matrix::invert()
{
	lapack_int ipiv[4];
	LAPACKE_sgetrf(LAPACK_ROW_MAJOR, 4, 4, (float*)this, 4, ipiv);
	LAPACKE_sgetri(LAPACK_ROW_MAJOR, 4, (float*)this, 4, ipiv);
}
void matrix::transpose()
{
	cblas_strsm(CblasRowMajor, CblasLeft, CblasUpper, CblasNoTrans, CblasNonUnit, 4, 4, 1, (float*)this, 4, (float*)this, 4);
}

void vector4::operator*=(const matrix&m)
{
	//inplace matrix multiplication
	vector4 result;
	cblas_sgemv(CblasRowMajor, CblasNoTrans, 4, 4, 1, (float*)&m, 4, (float*)this, 1, 0, (float*)&result, 1);
	memcpy(this, &result, 4*sizeof(float));
}

bool matrix::is_identity() const
{
	return row0 == vector4{1,0,0,0} && row1 == vector4{0,1,0,0} && row2 == vector4{0,0,1,0} && row3 == vector4{0,0,0,1};
}