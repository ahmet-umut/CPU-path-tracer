#include "Scene.hh"
#include <libxml2/libxml/parser.h>
#include <iostream>
#include <sstream>
#include "vector.hh"
#include "match.hh"
#include "read_array.hh"
#include "parse_image.hh"
#include "Tempor.hh"
#include "contains.hh"
using namespace std;

//static vector<int> texture_indices;

#include "accelerate.hh"
Scene::Scene(const char * filename)
{
	Tempor tempor;
	xmlDoc *doc = xmlReadFile(filename, NULL, XML_PARSE_HUGE);
	if (doc == NULL)
	{
		fprintf(stderr, "Could not parse the scene: %s\n", filename);
		exit(1);
	}
	for (bool finished=false; !finished;)
	{
		xmlNode *root_element = xmlDocGetRootElement(doc);
		xmlNode *cur_node = NULL;
		for (cur_node = root_element->children; cur_node; cur_node = cur_node->next)
		 if (cur_node->type == XML_ELEMENT_NODE)
		 {
			if (match(cur_node, "BackgroundColor"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				auto array = read_int_array(cur_node);
				background_color = {uint16_t(array[0]), uint16_t(array[1]), uint16_t(array[2])};
				parsed = true;
			}
			else if (match(cur_node, "MaxRecursionDepth"))
			{
				static bool parsed = false;
				if (!parsed)
				{
					max_recursion_depth = atoi((const char*)xmlNodeGetContent(cur_node));
					parsed = true;
				}
			}
			else if (match(cur_node, "ShadowRayEpsilon"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				shadow_ray_epsilon = atof((const char*)xmlNodeGetContent(cur_node));
				parsed = true;
			}
			else if (match(cur_node, "Cameras"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				for (xmlNode *camera_node = cur_node->children; camera_node; camera_node = camera_node->next)
				 if (camera_node->type == XML_ELEMENT_NODE && match(camera_node, "Camera"))
				 	cameras.emplace_back(camera_node);
				parsed = true;
			}
			else if (match(cur_node, "Lights"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				for (xmlNode *light_node = cur_node->children; light_node; light_node = light_node->next)
				if (light_node->type == XML_ELEMENT_NODE)
				{
					if (match(light_node, "AmbientLight"))
					{
						auto accumulator = read_int_array(light_node);
						ambient_light = 
						{
							uint16_t(accumulator[0]),
							uint16_t(accumulator[1]),
							uint16_t(accumulator[2])
						};
					}
					else if
					(
						match(light_node, "PointLight")
					)
						point_lights.emplace_back(light_node);
					else if
					(
						match(light_node, "AreaLight")
					)
						area_lights.emplace_back(light_node);
					else if (match(light_node, "SphericalDirectionalLight"))
					{
						if (xmlStrcmp(xmlGetProp(light_node, (const xmlChar*)"type"), (const xmlChar*)"latlong") == 0)
							background_type = latlong;
						else if (xmlStrcmp(xmlGetProp(light_node, (const xmlChar*)"type"), (const xmlChar*)"probe") == 0)
							background_type = probe;
						else	background_type = latlong;
						cout << "background type: " << background_type << endl;
						background.image_indice = atoi((const char*)xmlNodeGetContent(light_node))-1;
					}
					else if (match(light_node, "DirectionalLight"))
						directional_lights.emplace_back(light_node);
					else if (match(light_node, "SpotLight"))
						spot_lights.emplace_back(light_node);
					else
						cout << "Unknown light type: " << light_node->name << endl;
				}
				parsed = true;
			}
			else if (match(cur_node, "Materials"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				for (xmlNode *material_node = cur_node->children; material_node; material_node = material_node->next)
				if (material_node->type == XML_ELEMENT_NODE)
				{
					if (match(material_node, "Material"))
					{
						tempor.materials.emplace_back(material_node);
					}
				}
				parsed = true;
			}
			else if (match(cur_node, "Textures"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				for (xmlNode *texture_node = cur_node->children; texture_node; texture_node = texture_node->next)
				 if (texture_node->type == XML_ELEMENT_NODE)
				 {
					if (match(texture_node, "TextureMap"))
					{
						auto&texture = tempor.textures.emplace_back(texture_node);
						if (texture.type == Texture::background)
						{
							background_type = replace;
							background.image_indice = texture.image_index;
						}
					}
					else if (match(texture_node, "Images"))
					{
						for (xmlNode *image_node = texture_node->children; image_node ; image_node = image_node->next)
						if (image_node->type == XML_ELEMENT_NODE)
						{
							images.push_back(parse_image(image_node));
						}
					}
				 }
				parsed = true;
			}
			else if (match(cur_node, "VertexData"))
			{
				static bool parsed = false;
				if (parsed)	continue;
    			//<VertexData binaryFile="geometry/vertices.bin" />
				char *binary_file = (char*)xmlGetProp(cur_node, (const xmlChar*)"binaryFile");
				if (!binary_file)
				{
					auto floats = read_float_array(cur_node);
					for (size_t i=0; i<floats.size(); i+=3)
						tempor.vertices.push_back({floats[i], floats[i+1], floats[i+2]});
				}
				else
				{
					FILE *file = fopen(binary_file, "rb");
					if (!file)
					{
						cerr << "Could not open binary file: " << binary_file << endl;
						exit(1);
					}
					fseek(file, 0, SEEK_END);
					size_t size = ftell(file)-sizeof(int);
					fseek(file, sizeof(int), SEEK_SET);
					tempor.vertices.resize(size/sizeof(vector3));
					cout << fread(tempor.vertices.data(), sizeof(vector3), size/sizeof(vector3), file) << " vertices read" << endl;
					fclose(file);
				}
				vertices = tempor.vertices;
				parsed = true;
			}
			else if (match(cur_node, "TexCoordData"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				tempor.texture_indices = read_int_array(cur_node);
				parsed = true;
			}
			else if (match(cur_node, "Transformations"))
			{
				static bool parsed = false;
				if (parsed)	continue;
				//parse_transformations(cur_node, tempor.transformations);
				for (xmlNode* child = cur_node->children; child; child = child->next)
				 if (child->type == XML_ELEMENT_NODE)
				 {
					tempor.add_transformation(child);
				 }
				parsed = true;
			}
			else if (match(cur_node, "Objects"))
			{
				static bool parsed = false;
				if (parsed)	break;
				for (xmlNode *object_node = cur_node->children; object_node; object_node = object_node->next)
				if (object_node->type == XML_ELEMENT_NODE)
				{
					if (match(object_node, "Sphere") || match(object_node, "LightSphere"))
						spheres.emplace_back(object_node, tempor);
					else if (match(object_node, "Triangle")) {
						//triangles.push_back(parse_triangle(object_node, vertices, materials, tempor.transformations));
						triangles.emplace_back(object_node, tempor);
					}
					else if (match(object_node, "Mesh"))
						meshes.emplace_back(object_node, tempor, triangle_count);
					else if (match(object_node, "MeshInstance"))
						//cout << "reading MeshInstance" << endl,
						meshes.emplace_back(object_node, tempor, meshes);
					/*
					<LightMesh id="1"> 
					<Material>1</Material>
					<Radiance>31.831 31.831 31.831</Radiance>
					<Faces vertexOffset="11">
						0 2 1
						2 0 3 
						3 4 7
						4 3 0
						1 6 5
						6 1 2
						4 5 6
						6 7 4
						0 5 4
						5 0 1
					</Faces>
					</LightMesh>
					*/
					else if (match(object_node, "LightMesh"))
						meshes.emplace_back(object_node, tempor, triangle_count);
					else
						cout << "Unknown object type: " << object_node->name << endl;
				}
				//cout << "Objects parsed" << endl;
				parsed = true;
				finished=true;
			}
		 }
	}
	/* vector<Triangle*> triangle_ptrs;
	for (auto&triangle : triangles)
	{
		triangle_ptrs.push_back(&triangle);
		cout << "triangle " << endl;
	}
	accelerate5(triangle_ptrs); */

	//accelerate3(meshes, gigabox);
	cout << "triangle count in the scene: " << triangle_count << endl;
}
