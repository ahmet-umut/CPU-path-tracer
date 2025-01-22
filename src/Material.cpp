#include "Material.hh"
#include "read_array.hh"
#include "match.hh"
Material::Material(xmlNode*node)
{
	vector<float> faccuf;
	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			faccuf = read_float_array(cur_node);
			if (match(cur_node, "PhongExponent"))
				phong_exponent = faccuf[0];
			else if (match(cur_node, "AmbientReflectance"))
				ambient = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "DiffuseReflectance"))
				diffuse = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "SpecularReflectance"))
				specular = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "MirrorReflectance"))
				mirror = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "AbsorptionCoefficient"))
				AbsorptionCoefficient = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "RefractionIndex"))
				refraction_index = faccuf[0];
		}
	}
}