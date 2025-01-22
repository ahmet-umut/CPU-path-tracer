#pragma once

#include <vector>
#include <string>
#include <cstdint> // For fixed-width integers
//#include <eigen3/Eigen/Dense>
#include "vector.hh"

// Structure to store face data
struct Face {
    uint8_t num_vertices; // Number of vertices in the face, typically 3 for triangles
    std::vector<int> indices;
};

// Function declarations
bool readPLY(const std::string& filename, std::vector<vector3>& vertices, std::vector<Face>& faces, std::vector<vector3>& scene_vertices);