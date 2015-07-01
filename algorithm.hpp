#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <CL/cl.hpp>

#include "graphics_factory.hpp"
#include "image.hpp"
#include "util.hpp"

struct Volume
{
    int* voxels = NULL;
    int cube_width;
};

class Algorithm
{
    public:
        Algorithm();
        ~Algorithm();
        void initialise(GraphicsFactory* graphics_factory, unsigned int width, unsigned int height);
        void generateDisparityMap(Image* left, Image* right, unsigned int window_size, Image* disparity_map);
        void convertDisparityMapToDepthMap(Image* disparity_map, int focal_length, int baseline_mm, Image* depth_map);
        void trackCamera(Image* depth_map, Image* vertex_map, Image* normal_map, const Util::CameraConfig& camera_config, const Util::Transformation& transformation);
        void tempSetVoxels(Image* image);
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

        cl::Buffer clBuffer_correspondences;

        Volume volume;
        Image* prev_vertex_map;
        Image* prev_normal_map;
};

#endif
