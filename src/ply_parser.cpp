#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include "vector.hh"
#include "ply_parser.h"

#include <fstream>
#include <cstdint>
#include <cstdlib>

#include "happly.h"

using namespace std;

bool readPLY(
    const std::string& filename,
    std::vector<vector3>& vertices,
    std::vector<Face>& faces,
    std::vector<vector3>& scene_vertices
) {
    happly::PLYData ply(filename);
    cout << "Reading PLY file: " << filename << "\t" << flush;

    auto ply_vertices = ply.getVertexPositions();
    auto ply_faces = ply.getFaceIndices();

    for (size_t i = 0; i < ply_vertices.size(); i++) {
        vertices.push_back(vector3(ply_vertices[i][0], ply_vertices[i][1], ply_vertices[i][2]));
        scene_vertices.push_back(vector3(ply_vertices[i][0], ply_vertices[i][1], ply_vertices[i][2]));
    }

    for (size_t i = 0; i < ply_faces.size(); i++) {
        Face face;
        /* face.num_vertices = 3;
        face.indices.push_back(ply_faces[i][0]);
        face.indices.push_back(ply_faces[i][1]);
        face.indices.push_back(ply_faces[i][2]);
        faces.push_back(face); */
        for (size_t j = 0; j < ply_faces[i].size(); j++) {
            face.indices.push_back(ply_faces[i][j]);
        }
        faces.push_back(face);
    }

    return true;
}