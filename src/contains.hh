#include "vector.hh"
inline bool contains(const vector3 min, const vector3 max, const vector3 point)
{
	return
		point.x >= min.x && point.x <= max.x &&
		point.y >= min.y && point.y <= max.y &&
		point.z >= min.z && point.z <= max.z;
}