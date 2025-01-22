#include "submit.hh"

#include "tinyexr.h"
#include <png.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include <cmath>

#include "vector.hh"

using namespace std;
void saveToEXR(void* data, int xres, int yres, const std::string& filename) {
    auto image = (vector3(*)[yres])data;
    std::vector<float> exrData(4 * xres * yres);

	cout << "xres: " << xres << " yres: " << yres << endl;

    for (int i = 0; i < yres; ++i) {
        for (int j = 0; j < xres; ++j) {
            /* int idx = i * xres + j;
			cout << image[idx].x << " " << image[idx].y << " " << image[idx].z << endl;
            exrData[4 * idx + 0] = image[idx].x; // R
            exrData[4 * idx + 1] = image[idx].y; // G
            exrData[4 * idx + 2] = image[idx].z; // B */
			int idx = i * xres + j;
			// << image[j][i].x << " " << image[j][i].y << " " << image[j][i].z << endl;
			exrData[4 * idx + 0] = image[j][i].x; // R
			exrData[4 * idx + 1] = image[j][i].y; // G
			exrData[4 * idx + 2] = image[j][i].z; // B
            exrData[4 * idx + 3] = 1.0f;         // Alpha
        }
    }

    const char* err;
    int ret = SaveEXR(reinterpret_cast<const float*>(exrData.data()), xres, yres, 4, /*save as RGBA*/ false, filename.c_str(), &err);

    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "Failed to save EXR: " << err << std::endl;
        FreeEXRErrorMessage(err);
        throw std::runtime_error("Failed to save EXR image");
    }
}

void saveToPNG(void* data, int xres, int yres, const std::string& filename) {
    auto image = (vector3(*)[yres])data;

    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Failed to open file for writing PNG");
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        throw std::runtime_error("Failed to create PNG write structure");
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        throw std::runtime_error("Failed to create PNG info structure");
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        throw std::runtime_error("Failed during PNG creation");
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, xres, yres, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    std::vector<png_byte> row(3 * xres);
    for (int i = 0; i < yres; ++i) {
        for (int j = 0; j < xres; ++j) {
            int idx = i * xres + j;
            /* row[3 * j + 0] = static_cast<png_byte>(rint(image[idx].x));
			row[3 * j + 1] = static_cast<png_byte>(rint(image[idx].y));
			row[3 * j + 2] = static_cast<png_byte>(rint(image[idx].z)); */
			row[3 * j + 0] = static_cast<png_byte>(rint(image[j][i].x));
			row[3 * j + 1] = static_cast<png_byte>(rint(image[j][i].y));
			row[3 * j + 2] = static_cast<png_byte>(rint(image[j][i].z));
        }
        png_write_row(png, row.data());
    }

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}
