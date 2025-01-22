#include "Texture.hh"
#include "match.hh"
#include "read_array.hh"
#include <iostream>
#include <string>
using std::cout, std::endl, std::string;
Texture::Texture(xmlNode*node)
{
	char*typeprop = (char*)xmlGetProp(node, (const xmlChar*)"type");
	if (typeprop)
	{
		string type = typeprop;
		if (type == "image")	source=image;
		else if (type == "checkerboard")	source=kareli;
		else cout << "invalid texture type=" << typeprop << endl;
	}
	else cout << "no type property in texture" << endl;

	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next)
	 if (cur_node->type == XML_ELEMENT_NODE)
	 {
		/* if (match(cur_node, "NoiseConversion"))
		{
			string noicon = (const char*)xmlNodeGetContent(cur_node);
			//if (noicon == "absval")	return {Texture::noicon, Texture::absval};
			if (noicon == "absval")	type=noise, noise_conversion=absval;
			else if (noicon == "linear")	type=noise, noise_conversion=linear;
			else cout << "invalid noise conversion was parsed" << endl;
		}
		else */ if (match(cur_node, "ImageId"))
		{
			image_index = atoi((const char*)xmlNodeGetContent(cur_node)-1);
		}
		else if (match(cur_node, "DecalMode"))
		{
			string decal = (const char*)xmlNodeGetContent(cur_node);
			if (decal == "bump_normal")	type=bump;
			else if (decal == "replace_all")	type=all;
			else if (decal == "replace_kd")	type=kd_replace;
			else if (decal == "blend_kd")	type=kd_blend;
			else if (decal == "replace_background")	type=background;
			else cout << "invalid decal mode during texture parse: " << decal << endl;
		}
		else if (match(cur_node, "Interpolation"))
		{
			string interp = (const char*)xmlNodeGetContent(cur_node);
			if (interp == "bilinear")	interpolation=bilinear;
			else cout << "invalid interpolation setting during texture parse: " << interp << endl;
		}
		else if (match(cur_node, "Normalizer"))
		{
			normalizer = atof((const char*)xmlNodeGetContent(cur_node));
		}
		/*
		        <TextureMap id="4" type="checkerboard">
            <BlackColor>0.02 0.02 0.02</BlackColor>
            <WhiteColor>0.98 0.98 0.98</WhiteColor>
            <Scale>5</Scale>
            <Offset>0</Offset>
            <DecalMode>replace_kd</DecalMode>
        </TextureMap>
		*/
		/* else if (match(cur_node, "BlackColor"))
		{
			black = read_float_array(cur_node);
		}
		else if (match(cur_node, "WhiteColor"))
		{
			white = read_float_array(cur_node);
		}
		else if (match(cur_node, "Scale"))
		{
			scale = atof((const char*)xmlNodeGetContent(cur_node));
		}
		else if (match(cur_node, "Offset"))
		{
			offset = atof((const char*)xmlNodeGetContent(cur_node));
		} */
		else	cout << "no category identifier match during texture parse: " << cur_node->name << endl;
	 }
}
vector3 Texture::getcolor(vector3 coords, std::vector<std::vector<std::vector<vector3>>> &images)
{
	if (coords.x < 0 || coords.x > 1 || coords.y < 0 || coords.y > 1)
	{
		cout << "texture::getcolor: Texture coordinates out of bounds" << endl;
		cout << "coords: " << coords.x << " " << coords.y << endl;
		exit(1);
	}
	switch (source)
	{
	case image:
		{
			float posxf = (images[image_index][0].size()-1) * coords.x;
			float posyf = (images[image_index].size()-1) * coords.y;
			vector3 color(0,0,0);
			auto& image = images[image_index];
			switch (interpolation)
			{
			case bilinear:
				{
				//bool coin = rand() % 2;
				float dx = fmodf(posxf, 1), dy = fmodf(posyf, 1);
				color = color + image[floor(posyf)][floor(posxf)] * (1-dx) * (1-dy);
				color = color + image[floor(posyf)][ceil(posxf)] * dx * (1-dy);
				color = color + image[ceil(posyf)][floor(posxf)] * (1-dx) * dy;
				color = color + image[ceil(posyf)][ceil(posxf)] * dx * dy;
				}
				break;
			
			default:
				break;
			}
			return color;
		}
		break;
	case kareli:
		{
			int dim = 10;
			if ((int)(coords.x * dim) % 2 == (int)(coords.y * dim) % 2)
				return {0,0,0};
			else
				return {1,1,1};
			break;
		}
	default:
		//cout << "Texture type not implemented" << endl;
		//exit(1);
		return {0,0,0};
		break;
	}
	cout << "This should not be reached" << endl;
	exit(1);
}