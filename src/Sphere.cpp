#include "objects.hh"
#include "match.hh"
#include "read_array.hh"
#include "parse_transformation.hh"
#include "vector_utilities.hh"
Sphere::Sphere(xmlNode*node, Tempor&tempor)
{
	static unsigned char id=0;
	this->objectid=id++;

	vector<Texture> t;

    xmlNode *cur_node = NULL;
    for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Center")) {
                int index = atoi((const char*)xmlNodeGetContent(cur_node)) - 1;
                if (index >= 0 && index < tempor.vertices.size()) {
                    center = tempor.vertices[index];
                }
            } else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Radius")) {
                //radius = parse_float((const char*)xmlNodeGetContent(cur_node));
				radius = atof((const char*)xmlNodeGetContent(cur_node));
            } else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Material")) {
                int mat_index = atoi((const char*)xmlNodeGetContent(cur_node)) - 1;
                if (mat_index >= 0 && mat_index < 100) {
                    material = tempor.materials[mat_index];
                }
            }
			else if (match(cur_node, "Textures"))
			{
				auto indices = read_int_array(cur_node);
				for (size_t i = 0; i < indices.size(); i++)
					if (indices[i]--, indices[i] >= 0 && indices[i] < tempor.textures.size())
						textures.push_back(tempor.textures[indices[i]]);
			}
            else if (match(cur_node, "Transformations"))
            {
                transformation = parse_transformation(cur_node, tempor);
                inverse = transformation;
                lapack_int ipiv[4];
                LAPACKE_sgetrf(LAPACK_ROW_MAJOR, 4, 4, (float*)&inverse, 4, ipiv);
                LAPACKE_sgetri(LAPACK_ROW_MAJOR, 4, (float*)&inverse, 4, ipiv);
            }
            else if (match(cur_node, "Radiance"))
            {
                radiance = read_float_array(cur_node);
                is_lightsource = true;
            }
        }
    }
}
#include "Ray.hh"
#include "sphere2d.hh"
vector3 Sphere::get2dcoordinates(vector3 point)
{
    return sphere2dlatlong(point, center, radius, true);    //can also take this as the main function's parameter.
}