#pragma once
#include <vector>
#include "objects.hh"
int accelerate1(std::vector<Triangle>&triangles, AlignedBox& scene_box);
void accelerate3(std::vector<Mesh>&meshes, AlignedBox& gigabox);
AlignedBox*accelerate5(std::vector<Triangle*>triangles);