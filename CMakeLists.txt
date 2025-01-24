# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)


# Project name and specify C++ version
project(raytracer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(disabled_warnings "-Wno-narrowing -Wno-unused-result")

# Enable debugging symbols and disable narrowing warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${disabled_warnings} -fopenmp -march=native -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -march=native -g")

# Find required libraries
find_package(X11 REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(PNG REQUIRED)  # Find libpng
find_package(JPEG REQUIRED)  # Find libjpeg
find_package(OpenEXR REQUIRED)  # Find OpenEXR

# Check for OpenEXR and include its directories
if (OpenEXR_FOUND)
    include_directories(${OpenEXR_INCLUDE_DIRS})
    message(STATUS "OpenEXR found: ${OpenEXR_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "OpenEXR not found.")
endif()

# Add MKL support
set(MKL_DIR "/opt/intel/oneapi/mkl/latest") # Replace with your MKL installation path
set(CMAKE_PREFIX_PATH "/opt/intel/oneapi/mkl/latest/lib/cmake/mkl")
find_package(MKL CONFIG REQUIRED)

# Add the executable
file(GLOB SOURCES "src/*.cpp") # Collect all .cpp files in the directory
add_executable(raytracer ${SOURCES} miniz.o)

# Include directories for linked libraries
target_include_directories(raytracer PRIVATE 
    ${X11_INCLUDE_DIR} 
    ${LIBXML2_INCLUDE_DIR} 
    ${PNG_INCLUDE_DIRS} 
    ${JPEG_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(raytracer PRIVATE 
    ${X11_LIBRARIES} 
    ${LIBXML2_LIBRARIES} 
    ${PNG_LIBRARIES} 
    ${JPEG_LIBRARIES} 
    MKL::MKL
) 

# Output status for debugging
message(STATUS "X11 include dir: ${X11_INCLUDE_DIR}")
message(STATUS "X11 libraries: ${X11_LIBRARIES}")
message(STATUS "LibXml2 include dir: ${LIBXML2_INCLUDE_DIR}")
message(STATUS "LibXml2 libraries: ${LIBXML2_LIBRARIES}")
message(STATUS "PNG include dirs: ${PNG_INCLUDE_DIRS}")
message(STATUS "JPEG include dirs: ${JPEG_INCLUDE_DIR}")
message(STATUS "MKL found: ${MKL_FOUND}")
