#include "Scene.hh"
#include "match.hh"
#include "read_array.hh"

static double calculateNearPlaneHeight(double fovYDegrees, double nearDistance) {
    double fovYRadians = fovYDegrees * M_PI / 180.0;
    double height = 2 * nearDistance * tan(fovYRadians / 2);
    return height;
}

static double calculateNearPlaneWidth(double nearPlaneHeight, double aspectRatio) {
    double width = nearPlaneHeight * aspectRatio;
    return width;
}
#include <iostream>
using namespace std;
Camera::Camera(xmlNode*node)
{
	vector3 gazepoint;
	//char image_name[100];
	
	vector<float> faccuf;
	vector<int> iaccui;

	float fovy=0;

	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (match(cur_node, "Position"))
				faccuf = read_float_array(cur_node),
				position = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "Gaze"))
				faccuf = read_float_array(cur_node),
				gaze = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "GazePoint"))
			{
				faccuf = read_float_array(cur_node),
				 gazepoint = {faccuf[0], faccuf[1], faccuf[2]};
				vsSub(3, (float*)&gazepoint, (float*)&position, (float*)&gaze);
			}
			else if (match(cur_node, "Up"))
				faccuf = read_float_array(cur_node),
				up = {faccuf[0], faccuf[1], faccuf[2]};
			else if (match(cur_node, "NearPlane"))
				faccuf = read_float_array(cur_node),
				near_plane[0] = faccuf[0],
				near_plane[1] = faccuf[1],
				near_plane[2] = faccuf[2],
				near_plane[3] = faccuf[3];
			else if (match(cur_node, "FovY"))
			{
				fovy = atof((const char*)xmlNodeGetContent(cur_node));
			}
			else if (match(cur_node, "NearDistance"))
				near_distance = atof((const char*)xmlNodeGetContent(cur_node));
			else if (match(cur_node, "ImageResolution"))
				iaccui = read_int_array(cur_node),
				image_resolution[0] = iaccui[0], image_resolution[1] = iaccui[1];
			else if (match(cur_node, "ImageName"))
				{
					//remove the extension
					const char *name = (const char*)xmlNodeGetContent(cur_node);
					int i=0;
					for (; name[i] && name[i]!='.'; i++)
						image_name.push_back(name[i]);
				}
			else if (match(cur_node, "NumSamples"))
				sample_count = atoi((const char*)xmlNodeGetContent(cur_node));
			else if (match(cur_node, "FocusDistance"))
				focus_distance = atof((const char*)xmlNodeGetContent(cur_node));
			else if (match(cur_node, "ApertureSize"))
				aperture_size = atof((const char*)xmlNodeGetContent(cur_node));
			else if (match(cur_node, "Tonemap"))
				for (xmlNode *tone_node = cur_node->children; tone_node; tone_node = tone_node->next)
				{
					hdr=true;
					/*
					<TMOOptions>0.01 2</TMOOptions> <!-- key_value burn_percent -->
					<Saturation>1.0</Saturation>
					<Gamma>2.2</Gamma>
					*/
					if (match(tone_node, "TMOOptions"))
					{
						faccuf = read_float_array(tone_node);
						key_value = faccuf[0];
						burn_percent = faccuf[1];
					}
					else if (match(tone_node, "Saturation"))
						saturation = atof((const char*)xmlNodeGetContent(tone_node));
					else if (match(tone_node, "Gamma"))
						gamma = atof((const char*)xmlNodeGetContent(tone_node));
				}
			else if (match(cur_node, "Renderer"))
			{
				if (strcmp((const char*)xmlNodeGetContent(cur_node), "PathTracing")==0)
					pathtracing=true;
			}
			else if (match(cur_node, "RendererParams"))
			{
				if (strstr((const char*)xmlNodeGetContent(cur_node), "NextEventEstimation"))
					nee=true;
				//<RendererParams>ImportanceSampling</RendererParams>
				if (strstr((const char*)xmlNodeGetContent(cur_node), "ImportanceSampling"))
					importance_sampling=true;
			}
			else if (match(cur_node, "SplittingFactor"))
			{
				splitCount = atoi((const char*)xmlNodeGetContent(cur_node));
			}
		}
	}

	if (fovy>0)
	{
		double nearPlaneHeight = calculateNearPlaneHeight(fovy, near_distance);
		double nearPlaneWidth = calculateNearPlaneWidth(nearPlaneHeight, (double)image_resolution[0] / image_resolution[1]);
		near_plane[0] = -nearPlaneWidth / 2;
		near_plane[1] = nearPlaneWidth / 2;
		near_plane[2] = -nearPlaneHeight / 2;
		near_plane[3] = nearPlaneHeight / 2;
		//cout << "Near plane: " << near_plane[0] << " " << near_plane[1] << " " << near_plane[2] << " " << near_plane[3] << endl;
	}
	
	gaze.normalize();
}
