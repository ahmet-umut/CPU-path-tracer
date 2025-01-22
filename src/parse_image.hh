#pragma once
#include <vector>
#include <libxml/parser.h>
#include "vector.hh"
std::vector<std::vector<vector3>> parse_image(xmlNode *node);