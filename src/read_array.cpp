#include "read_array.hh"
#include <libxml2/libxml/parser.h>
#include <cstring>
vector<int> read_int_array(xmlNode*node)
{
	vector<int> accumulator;
	char *content = (char*)xmlNodeGetContent(node);
	for (char *token = strtok(content, " \n"); token != NULL; token = strtok(NULL, " \n"))
		// Skip empty lines or lines that contain only whitespace
		if (*token != '\0' && strspn(token, " \t") != strlen(token))
			accumulator.push_back(atoi(token));
    xmlFree(content);
	return accumulator;
}
vector<float> read_float_array(xmlNode*node)
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