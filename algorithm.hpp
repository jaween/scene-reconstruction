#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <CL/cl.hpp>

#include "image.hpp"
#include "util.hpp"

struct Volume
{
    int* voxels;
    int cube_width;
};

class Algorithm
{
    public:
        Algorithm();
        ~Algorithm();
        void generateDisparityMap(Image* left, Image* right, unsigned int window_size, Image* disparity_map);
        void convertDisparityMapToDepthMap(Image* disparity_map, int focal_length, int baseline_mm, Image* depth_map);
        void trackCamera(Image* depth_map, Image* vertex_map, Image* normal_map, const Util::CameraConfig& camera_config, int* transformation);
        void tempSetVoxels(Image* image);
        void initialiseVolume(int cube_width);
        void render(int eye_x, int eye_y, int eye_z, int screen_z, float angle, float distance, Image* screen);

    private:
        void initialiseOpenCL();
        std::string loadSource(std::string filename);
        void executeImageKernel(cl::Kernel& kernel, cl::Image2D& out_buffer, Image* out_image);

        cl::Device device;
        cl::Context context;
        cl::CommandQueue command_queue;
        cl::Program::Sources sources;
        cl::Program program;
        cl::Buffer buffer_voxels;
        Volume volume;

        cl::Image2D clImage_disparity;
};

#endif
