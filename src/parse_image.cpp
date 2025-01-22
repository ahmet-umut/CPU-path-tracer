#include "parse_image.hh"
#include <iostream>
#include <fstream>
#include <jpeglib.h>
#include <png.h>

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

using namespace std;

// Helper function to read binary data
template<typename T>
void readBinary(std::ifstream& stream, T& value) {
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));
}

// Function to read JPEG images

// Function to read JPEG images
static bool readJPEG(const std::string& filename, vector<vector<vector3>>& image) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE* infile = fopen(filename.c_str(), "rb");
    if (!infile) {
        cerr << "Cannot open file " << filename << endl;
        return false;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    unsigned long width = cinfo.output_width;
    unsigned long height = cinfo.output_height;
    unsigned short depth = cinfo.output_components;

    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width * depth, 1);

    image.resize(height);
    for (unsigned long y = 0; y < height; ++y) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        image[y].resize(width);
        for (unsigned long x = 0; x < width; ++x) {
            vector3 color(
                buffer[0][x * depth + 0],
                buffer[0][x * depth + (depth > 1 ? 1 : 0)],
                buffer[0][x * depth + (depth > 2 ? 2 : 0)]);
            image[y][x] = color;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return true;
}

// Function to read PNG images
static bool readPNG(const std::string& filename, vector<vector<vector3>>& image) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        cerr << "Cannot open file " << filename << endl;
        return false;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(file);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(file);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(file);
        return false;
    }

    png_init_io(png_ptr, file);
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    // Read the image
    png_bytep row_pointers[height];
    for (png_uint_32 i = 0; i < height; i++) {
        row_pointers[i] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    png_read_image(png_ptr, row_pointers);

    image.resize(height);
    for (png_uint_32 y = 0; y < height; y++) {
        image[y].resize(width);
        for (png_uint_32 x = 0; x < width; x++) {
            png_bytep px = &(row_pointers[y][x * 3]);
            vector3 color(px[0], px[1], px[2]);
            image[y][x] = color;
        }
    }

    // Clean up
    for (png_uint_32 y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(file);

    return true;
}

// Function to read EXR images
bool readEXR(const std::string& filename, std::vector<std::vector<vector3>>& image) {
  const char* err = nullptr;

  // Load EXR file
  float* outRgba;  // Pointer to loaded RGBA data
  int width, height;

  int ret = LoadEXR(&outRgba, &width, &height, filename.c_str(), &err);
  if (ret != TINYEXR_SUCCESS) {
    if (err) {
      std::cerr << "Error reading EXR file: " << err << std::endl;
      FreeEXRErrorMessage(err);
    } else {
      std::cerr << "Unknown error reading EXR file." << std::endl;
    }
    return false;
  }

  // Resize image to match EXR dimensions
  image.resize(height, std::vector<vector3>(width));

  // Populate the image vector with RGB values from the EXR file
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int index = 4 * (y * width + x); // RGBA channels, skip A
      image[y][x] = {outRgba[index], outRgba[index + 1], outRgba[index + 2]};
      if (outRgba[index+3]!=1)
      {
        cout << "alpha channel is not 1" << endl;
        exit(1);
      }
    }
  }

  // Free the loaded data
  free(outRgba);

  return true;
}

// Function to read image based on file type
vector<vector<vector3>> parse_image(xmlNode *node) {
    vector<vector<vector3>> image;
    string filename = reinterpret_cast<const char*>(xmlNodeGetContent(node));
    string extension = filename.substr(filename.find_last_of('.') + 1);

    if (extension == "png") {
        if (!readPNG(filename, image)) {
            cerr << "Failed to read PNG image." << endl;
        }
    } else if (extension == "jpg" || extension == "jpeg") {
        if (!readJPEG(filename, image)) {
            cerr << "Failed to read JPEG image." << endl;
        }
    } else if (extension == "exr") {
        if (!readEXR(filename, image)) {
            cerr << "Failed to read EXR image." << endl;
        }
    } else {
        cerr << "Unsupported image format: " << extension << endl;
    }
    cout << "image size: " << image.size() << "x" << image[0].size() << endl;
    return image;
}
