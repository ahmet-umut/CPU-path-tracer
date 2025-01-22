#include "Scene.hh"
#include "read_array.hh"
#include "match.hh"
#include <iostream>
#include "vector_utilities.hh"
using namespace std;
DirectionalLight::DirectionalLight(xmlNode*node)
{
	/*
        <DirectionalLight id="1">
            <Direction>1 -0.8 -1</Direction>
            <Radiance>200 200 200</Radiance>
        </DirectionalLight>
	*/
	for (xmlNode *cur_node = node->children; cur_node; cur_node = cur_node->next)
	 if (cur_node->type == XML_ELEMENT_NODE)
	 {
		if (match(cur_node, "Direction"))
		{
			auto array = read_float_array(cur_node);
			xzangl = atan2(array[2], array[0]);
			yangl = atan2(array[1], sqrt(array[0]*array[0] + array[2]*array[2]));
		}
		else if (match(cur_node, "Radiance"))
		{
			auto array = read_float_array(cur_node);
			radiance = {array[0], array[1], array[2]};
			cout << "DirectionalLight: " << xzangl << " " << yangl << " " << radiance << endl;
		}
	 }
}