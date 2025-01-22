#include "objects.hh"
#include <iostream>
#include "Tempor.hh"
#include "match.hh"
#include "read_array.hh"
#include "ply_parser.h"
#include "parse_transformation.hh"
#include "accelerate.hh"
#include "vector_utilities.hh"
Mesh::Mesh(xmlNode*node, Tempor&tempor, uint&count)	//mesh
{
    static unsigned char id = 0;
    char plyFilePath[1024] = {0}; // Buffer to store the path to the .ply file

	//Matrix4f transformation = Matrix4f::Identity();

	vector<Triangle> triangles;

    xmlNode *cur_node = NULL;
    for (cur_node = node->children; cur_node; cur_node = cur_node->next)
	if (cur_node->type == XML_ELEMENT_NODE)
	{
		if (match(cur_node,"Material")) {
			int mat_index = atoi((const char*)xmlNodeGetContent(cur_node)) - 1;
			if (mat_index >= 0 && mat_index < tempor.materials.size())
			{
				material = tempor.materials[mat_index];
			}
		}
		else if (match(cur_node, "Textures"))
		{
			auto indices = read_int_array(cur_node);
			for (auto index : indices)
				if (index--, index >= 0 && index < tempor.textures.size())
					textures.push_back(tempor.textures[index]);
		/* 	for (size_t i = 0; i < indices.size(); i++)
				if (indices[i] >= 0 && indices[i] < tempor.textures.size())
					textures.push_back(tempor.textures[indices[i]]); */
		}
		else if (match(cur_node,"Faces"))
		{
			// Check if the Faces element has a 'plyFile' attribute
			xmlChar* plyFile = xmlGetProp(cur_node, (const xmlChar *)"plyFile");
			char* offsetstr = (char*)xmlGetProp(cur_node, (const xmlChar *)"vertexOffset");
			char* binaryFile = (char*)xmlGetProp(cur_node, (const xmlChar *)"binaryFile");

			int offset = offsetstr ? atoi(offsetstr) : 0;
			//cout << "offset: " << offset << endl;
			if (plyFile)
			{
				strncpy(plyFilePath, (const char*)plyFile, sizeof(plyFilePath) - 1);
				//printf("PLY file specified: %s\n", plyFilePath);
				xmlFree(plyFile);
				// We have a PLY file path, parse the PLY file
				std::vector<vector3> plyVertices;
				std::vector<Face> plyFaces;
				if (readPLY(plyFilePath, plyVertices, plyFaces, tempor.vertices)) {
					// Convert ply data to mesh triangles
					for (const auto& face : plyFaces) {
						for (int i = 2; i < face.indices.size(); i++)
						{
							unsigned char id=0;
							int i0 = face.indices[0]+offset, i1 = face.indices[i - 1]+offset, i2 = face.indices[i]+offset;
							triangles.emplace_back(id++, plyVertices[i0], plyVertices[i1], plyVertices[i2], material);
						}
					}
				} else {
					std::cerr << "Failed to parse PLY file: " << plyFilePath << std::endl;
					exit(1);
				}
			}
			else if (binaryFile)
			{
				FILE *file = fopen(binaryFile, "rb");
				if (!file)
				{
					cerr << "Could not open binary file: " << binaryFile << endl;
					exit(1);
				}
				fseek(file, 0, SEEK_END);
				size_t size = ftell(file)-sizeof(int);
				fseek(file, sizeof(int), SEEK_SET);
				for (size_t i=0; i<size; i+=3*sizeof(int))
				{
					int indices[3];
					fread(indices, sizeof(int), 3, file);
					indices[0]+=offset, indices[1]+=offset, indices[2]+=offset;
					triangles.emplace_back(id++, tempor.vertices[indices[0]], tempor.vertices[indices[1]], tempor.vertices[indices[2]], material);
				}
				fclose(file);
			}
			else
			{
				// Parse inline faces as before
				auto vertex_indices = read_int_array(cur_node);
				for (size_t i = 0; i < vertex_indices.size(); i += 3) {
					vertex_indices[i]--, vertex_indices[i + 1]--, vertex_indices[i + 2]--;
					if (i + 2 < vertex_indices.size())
					{
						int i0 = vertex_indices[i]+offset, i1 = vertex_indices[i + 1]+offset, i2 = vertex_indices[i + 2]+offset;
						triangles.emplace_back(id++, tempor.vertices[i0], tempor.vertices[i1], tempor.vertices[i2], material);

						if (tempor.texture_indices.size())//offset not implemented here
						{
							triangles.back().u1 = tempor.texture_indices[2*vertex_indices[i]],
							triangles.back().v1 = tempor.texture_indices[2*vertex_indices[i]+1],
							triangles.back().u2 = tempor.texture_indices[2*vertex_indices[i+1]],
							triangles.back().v2 = tempor.texture_indices[2*vertex_indices[i+1]+1],
							triangles.back().u3 = tempor.texture_indices[2*vertex_indices[i+2]],
							triangles.back().v3 = tempor.texture_indices[2*vertex_indices[i+2]+1];
						}
					}
				}
			}
		}
		else if (match(cur_node, "Transformations"))
		{
			transformation = parse_transformation(cur_node, tempor);
			inverse = transformation;	inverse.invert();
			transpose = inverse;	transpose.transpose();
		}
		else if (match(cur_node, "Radiance"))
		{
			radiance = read_float_array(cur_node);
			is_lightsource = true;
		}
    }
	objectid = id++;

	if (is_lightsource)
	{
		center = vector3(0,0,0);
		for (auto&triangle:triangles)
			center += triangle.vertex1;
		center = center / triangles.size();
	}

	vector<Triangle*> trianglesp;
	for (auto&triangle:triangles)
		trianglesp.push_back(&triangle);
	data = accelerate5(trianglesp);

	cout << trianglesp.size() << " triangles in mesh" << endl;
	count += trianglesp.size();
	/* cout << "data min: " << data->min << endl;
	cout << "data max: " << data->max << endl; */
}
Mesh::Mesh(xmlNode*node, Tempor&tempor, vector<Mesh>&meshes)	//mesh instance
{
	uint mesh_index = atoi((const char*)xmlGetProp(node, (const xmlChar *)"baseMeshId")) - 1;
	if (!(mesh_index >= 0 && mesh_index < meshes.size()))
		fprintf(stderr, "Warning: Mesh instance skipped due to invalid mesh index\n");
	data = meshes[mesh_index].data;

	const char* resetTransform = (const char*)xmlGetProp(node, (const xmlChar *)"resetTransform");
	if (resetTransform && string(resetTransform) == "true")
		transformation = matrix::identity();
	else
		//cout << "resetTransform not true" << endl,
		transformation = meshes[mesh_index].transformation;

	xmlNode *cur_node = NULL;
    for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if (match(cur_node, "Material")) {
                int mat_index = atoi((const char*)xmlNodeGetContent(cur_node)) - 1;
                if (mat_index >= 0 && mat_index < tempor.materials.size())
				{
                    material = tempor.materials[mat_index];
                }
				else	cout << "invalid material index: " << mat_index << endl;
            }
			else if (match(cur_node, "Transformations"))
			{
				auto t = parse_transformation(cur_node, tempor);
				transformation = t * transformation;
			}
        }
    }
	//cout << "mesh transformations" << endl << transformation << endl;
}