#pragma once
#include <vector>
#include <libxml2/libxml/parser.h>
using std::vector;
vector<int> read_int_array(xmlNode*node);
vector<float> read_float_array(xmlNode*node);
/*
static vector<int> read_int_array(xmlNode*node)
{
	vector<int> accumulator;
	char *content = (char*)xmlNodeGetContent(node);
	for (char *token = strtok(content, " \n"); token != NULL; token = strtok(NULL, " \n"))
		// Skip empty lines or lines that contain only whitespace
		if (*token != '\0' && strspn(token, " \t") != strlen(token))
			accumulator.push_back(atoi(token)-1);
    xmlFree(content);
	return accumulator;
}
static vector<float> read_float_array(xmlNode*node)
{
	vector<float> accumulator;
	char *content = (char*)xmlNodeGetContent(node);
	for (char *token = strtok(content, " \n"); token != NULL; token = strtok(NULL, " \n"))
		// Skip empty lines or lines that contain only whitespace
		if (*token != '\0' && strspn(token, " \t") != strlen(token))
			accumulator.push_back(atof(token));
    xmlFree(content);
	return accumulator;
}
*/