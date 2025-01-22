#pragma once
#include "Ray.hh"
#include "Scene.hh"
#include "vector.hh"
#include "XSystem.hh"
float cos_vv(const vector3& a, const vector3& b);
vector3 sendray(Scene&scene, Ray ray, bool verbose=false, uint depth=0, float pathlength=0, vector3 pixel={0,0,false}, Xsystem* = nullptr, Path* = nullptr);
vector3 clamp(vector3 color);