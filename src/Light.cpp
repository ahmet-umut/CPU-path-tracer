#include "Light.hh"
#include "vector_utilities.hh"
#include "read_array.hh"
#include "match.hh"

Ray _PointLight::sample_ray(const vector3 & start) const
{
	Ray ray(start, position-start);
	ray.length = norm(position - start);
	return ray;
}
vector3 _PointLight::getradiance(Ray & ray) const
{
	return intensity / powf(ray.length, 2);
}
_PointLight::_PointLight(xmlNode*node)
{
	vector<float> faccuf;
	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (match(cur_node, "Position"))
				faccuf = read_float_array(cur_node),
				position = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Intensity"))
				faccuf = read_float_array(cur_node),
				intensity = {faccuf[0], faccuf[1], faccuf[2]};
		}
	}
}