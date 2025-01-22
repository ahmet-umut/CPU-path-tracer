#pragma once
#include <libxml2/libxml/parser.h>
inline bool match(const xmlNode* node, const char* name)
{
	return !xmlStrcmp(node->name, (const xmlChar *)name);
}