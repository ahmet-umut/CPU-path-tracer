#include "Scene.hh"
#include "match.hh"
#include "read_array.hh"
PointLight::PointLight(xmlNode*node)
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
AreaLight::AreaLight(xmlNode*node)
{
	vector<float> faccuf;
	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			faccuf = read_float_array(cur_node);
			if (match(cur_node, "Position"))
				position = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Radiance"))
				radiance = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Normal"))
				normal = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Size"))
				size = atof((const char*)xmlNodeGetContent(cur_node));
		}
	}
}
SpotLight::SpotLight(xmlNode*node)
{
	/*<SpotLight id="1">
            <Position>-0.93 1 0.9</Position>
            <Direction>1 -1 -1</Direction>
            <Intensity>600 600 600</Intensity>
            <CoverageAngle>10</CoverageAngle>
            <FalloffAngle>8</FalloffAngle>
        </SpotLight>*/
	/*struct SpotLight
	{
		vector3 position, direction, intensity;
		float coverage_angle, falloff_angle;
		SpotLight(xmlNode*node);
	};*/
	vector<float> faccuf;
	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			faccuf = read_float_array(cur_node);
			if (match(cur_node, "Position"))
				position = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Direction"))
				direction = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Intensity"))
				intensity = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "CoverageAngle"))
				coverage_angle = atof((const char*)xmlNodeGetContent(cur_node)) / 180 * M_PI /2;
			else if (match(cur_node, "FalloffAngle"))
				falloff_angle = atof((const char*)xmlNodeGetContent(cur_node)) / 180 * M_PI /2;
		}
	}
};