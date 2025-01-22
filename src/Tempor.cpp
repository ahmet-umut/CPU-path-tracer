#include "Tempor.hh"
#include <string>
#include <sstream>
#include "read_array.hh"
#include "mkl.h"
#include <iostream>
#include "vector_utilities.hh"
using namespace std;
void Tempor::add_transformation(xmlNode*node)
{
	std::string nodeName = (const char*)node->name;

	matrix mat = matrix::identity();

	// Parse the vector components
	float a, x, y, z;
	std::string content = (const char*)xmlNodeGetContent(node);
	std::istringstream iss(content);
	
	if (nodeName == "Translation" || nodeName == "Scaling") {
		iss >> x >> y >> z;
		if (nodeName == "Translation") {
			// Create translation matrix
			/* mat(0, 3) = x;
			mat(1, 3) = y;
			mat(2, 3) = z; */
			mat.row0[3] = x;
			mat.row1[3] = y;
			mat.row2[3] = z;
			transformations[2].push_back(mat);
		} else if (nodeName == "Scaling") {
			// Create scaling matrix
			/* mat(0, 0) = x;
			mat(1, 1) = y;
			mat(2, 2) = z; */
			mat.row0[0] = x;
			mat.row1[1] = y;
			mat.row2[2] = z;
			transformations[1].push_back(mat);
		}
	} else if (nodeName == "Rotation") {
		iss >> a >> x >> y >> z;
		float c = cos(a/180*M_PI), s = sin(a/180*M_PI);
		vector3 aa = {x, y, z};
		aa.normalize();
		matrix a_hat;
		a_hat.row0 = {0, -aa.z, aa.y, 0};
		a_hat.row1 = {aa.z, 0, -aa.x, 0};
		a_hat.row2 = {-aa.y, aa.x, 0, 0};
		a_hat.row3 = {0, 0, 0, 0};
		matrix a_hat2 = a_hat * a_hat;
		mkl_somatadd('R', 'N', 'N', 4,4, s,&a_hat, 4, 1-c,&a_hat2, 4, &mat, 4);
		mat.row0[0]+=1, mat.row1[1]+=1, mat.row2[2]+=1, mat.row3[3]+=1;
		transformations[0].push_back(mat);
	}
	else if (nodeName == "Composite")
	{
		auto floats = read_float_array(node);
		for (size_t i = 0; i < floats.size(); i += 16)
		{
			for (size_t j = 0; j < 16; j++)
				//m(j / 4, j % 4) = floats[i + j];
				mat[j / 4][j % 4] = floats[i + j];
			/* cout << mat.row0 << endl;
			cout << mat.row1 << endl;
			cout << mat.row2 << endl;
			cout << mat.row3 << endl; */
		}
		transformations[3].push_back(mat);
	}
	//transformations[nodeName[0] - 'R'].push_back(mat);
}