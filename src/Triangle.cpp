#include "objects.hh"
#include <iostream>
#include "match.hh"
#include "parse_transformation.hh"
using namespace std;
Triangle::Triangle(xmlNode*node, Tempor&tempor)
{
	static unsigned char id=0;
	
	Material m;
	xmlNode *cur_node = NULL;
	int vertex_indices[3] = {-1, -1, -1}; // Initialize vertex indices to invalid values
	int mat_index = -1;  // Initialize Material index to invalid value

	matrix transformation = matrix::identity();

	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Indices")) {
                sscanf((const char*)xmlNodeGetContent(cur_node), "%d %d %d", &vertex_indices[0], &vertex_indices[1], &vertex_indices[2]);
                // Adjust indices for zero-based array indexing
                vertex_indices[0]--;
                vertex_indices[1]--;
                vertex_indices[2]--;
            } else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Material")) {
                int mat_index = atoi((const char*)xmlNodeGetContent(cur_node)) - 1;
                if (mat_index >= 0 && mat_index < 100) {
                    m = tempor.materials[mat_index];
                }
            }
			else if (match(cur_node, "Transformations"))
			{
				transformation = parse_transformation(cur_node, tempor);
			}
        }
    }

	// Verify all required data is present before writing
	if (vertex_indices[0] >= 0 && vertex_indices[1] >= 0 && vertex_indices[2] >= 0) {
		printf("writing triangle data\n");
	} else {
		fprintf(stderr, "Warning: Triangle skipped due to invalid vertex indices (Triangle::Triangle)\n");
	}
	//printf("Triangle parsed\n");

	*this = Triangle(id++, tempor.vertices[vertex_indices[0]], tempor.vertices[vertex_indices[1]], tempor.vertices[vertex_indices[2]], m);
	transform(transformation);

	cout << "Triangle " << id << " created with vertices " << vertex_indices[0] << ", " << vertex_indices[1] << ", " << vertex_indices[2] << endl;
	cout << "Triangle " << id << " created with vertices " << vertex1.x << ", " << vertex1.y << ", " << vertex1.z << endl;
}
vector3 Triangle::normal()
{
	auto _ = vertex2delta.cross(vertex3delta);
	_.normalize();
	return _;
}
#include "vector_utilities.hh"
vector3 Triangle::get2dcoordinates(vector3 point)
{
	auto v0 = vertex2delta;
	auto v1v3 = vertex3delta;
	auto v1p = point - vertex1;
	auto d00 = dot(v0, v0);
	auto d01 = dot(v0, v1v3);
	auto d11 = dot(v1v3, v1v3);
	auto d20 = dot(v1p, v0);
	auto d21 = dot(v1p, v1v3);
	auto denom = d00 * d11 - d01 * d01;
	if (denom == 0)
        throw std::invalid_argument("Triangle is degenerate or very close to degenerate.");
	double lambda2 = (d11 * d20 - d01 * d21) / denom;
	double lambda3 = (d00 * d21 - d01 * d20) / denom;
	double lambda1 = 1 - lambda2 - lambda3;

	vector3 uv = vector3(u1,v1,0) * lambda1 + vector3(u2,v2,0) * lambda2 + vector3(u3,v3,0) * lambda3;
	auto mod = [](float x) {return fmodf(fmodf(x, 1) + 1, 1);};
	uv = uv.apply(mod);

	if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
	{
		cout << "Texture coordinates out of bounds" << endl;
		cout << "coords: " << uv << endl;
		exit(1);
	}
	return uv;
}