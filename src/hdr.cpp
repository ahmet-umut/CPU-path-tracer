#include "hdr.hh"
#include "state.hh"
#include <iostream>
#include "vector_utilities.hh"
#include <algorithm>
using namespace std;
static vector3 clamp(vector3 color, float maximum=255)
{
	static const vector3 maxcolor{maximum,maximum,maximum};
	vector3 result;
	vsFmin(3, (float*)&maxcolor, &color, &result);
	return result;
}
static float luminance(vector3 color)
{
	return color.x * 0.2 + color.y * 0.7 + color.z * 0.1;
}

void applyHDRTonemapping(Xsystem xsystem, int xresolution, int yresolution, void*pointer)
{
	auto image = (vector3(*)[yresolution])pointer;
	float lw=0;

	float luminances[yresolution*xresolution];
	for (unsigned int y = 0; y < yresolution; y++)
		for (unsigned int x = 0; x < xresolution; x++)
			luminances[y * xresolution + x] = luminance(image[x][y]),
			#define delta 1e-0
			lw += log(luminances[y * xresolution + x] + delta) / xresolution / yresolution;
	std::sort(luminances, luminances + xresolution * yresolution);
	cout << "HDR.lw.0: " << lw << endl;
	lw = exp(lw);
	cout << "HDR.lw.1: " << lw << endl;

	float lwhite=INFINITY;
	//if (xsystem.camera.burn_percent)
		lwhite = luminances[lrint((xresolution * yresolution * (100-xsystem.camera.burn_percent)) / 100)];
	cout << "HDR.lwhite: " << lwhite << endl;

	/* for (auto&luminance:luminances)
	{
		static bool okay=true;
		if (luminance <= 0)	;
		else	lw += log(luminance) / xresolution / yresolution;
		if (lw==lw)	cout << "lw: " << lw << " luminance: " << luminance << "  \r";
		else if (okay)
		{
			cout<<endl;
			cout << "lw is nan" << endl;
			cout << "lw: " << lw << " luminance: " << luminance << endl;
			okay=false;
		}
	}
	cout <<endl;
	cout << "HDR.lw.0: sum of log(1e-9 + luminance(image[x][y])): " << lw << endl;
	lw = exp(lw);
	cout << "HDR.lw.1: exp(lw.0 / (xresolution * yresolution)): " << lw << endl; */

	for (unsigned int y = 0; y < yresolution; y++)
		for (unsigned int x = 0; x < xresolution; x++)
		{
			float lwxy = luminance(image[x][y]);
			float l = luminance(image[x][y]) * xsystem.camera.key_value / lw;
			//float ld = l / (1 + l);
			float ld = l * (1 + l/lwhite/lwhite) / (1+l);
			vector3 color = image[x][y];
			auto saturate = [ld,lwxy,xsystem](float x) {if (ld==0) return 0.f; return (float)(ld * powf(x/lwxy, xsystem.camera.saturation));};
			color = color.apply(saturate);
			auto clamp2 = [](float x) {if (x<0) return 0.f; if (x>1) return 1.f; return x;};
			color = color.apply(clamp2);
			auto gamma = [xsystem](float x) {return powf(x, 1/xsystem.camera.gamma);};
			color = color.apply(gamma);
			color = clamp(color*255);
			#ifdef _xdebug
			xsystem.imagedata[y * xresolution + x] = ((unsigned int)color.x<<16) + ((unsigned int)color.y<<8) + (unsigned int)color.z;
			#endif
			image[x][y] = color;
		}
}