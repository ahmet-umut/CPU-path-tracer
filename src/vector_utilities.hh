#pragma once
#include "vector.hh"
inline float norm(const vector3& v)
{
	return cblas_snrm2(3, (float*)&v, 1);
}
inline void normalized(const vector3& vin, vector3& vout)
{
	cblas_scopy(3, (float*)&vin, 1, (float*)&vout, 1);
	cblas_sscal(3, 1.f / norm(vout), (float*)&vout, 1);
}
inline float dot(const vector3& v1, const vector3& v2)
{
	return cblas_sdot(3, (float*)&v1, 1, (float*)&v2, 1);
}
inline vector3 normalized(const vector3& v)
{
	vector3 result;
	normalized(v, result);
	return result;
}

inline std::ostream& operator<<(std::ostream& os, const vector3& v)
{
	os << v.x << " " << v.y << " " << v.z;
	return os;
}
inline std::ostream& operator<<(std::ostream& os, const vector4& v)
{
	os << v.x << " " << v.y << " " << v.z << " " << v.x3;
	return os;
}