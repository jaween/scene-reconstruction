#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <CL/cl.hpp>
#include <ctime>

#include "image.hpp"
#include "util.hpp"

struct Volume
{
    int* voxels;
    int volume_size;
};

class Algorithm
{
    public:
        Algorithm();
        void generateDisparityMap(Image* left, Image* right, unsigned int window_size, Image* disparity_map);
        void convertDisparityMapToDepthMap(Image* disparity_map, int focal_length, int baseline_mm, Image* depth_map);
        void trackCamera(Image* depth_map, Util::CameraConfig* camera_config, Image* vertex_map, Image* normal_map, int* transformation);
        void setVolume(int* voxels, int volume_size);
        void render(int eye_x, int eye_y, int eye_z, int screen_z, float angle, float distance, Image* screen);

    private:
        void initialiseOpenCL();
        std::string loadSource(std::string filename);
        void executeImageKernel(cl::Kernel& kernel, cl::Image2D& out_buffer, Image* out_image);
        void startTimer();
        void endTimer(std::string function);

        cl::Device device;
        cl::Context context;
        cl::CommandQueue command_queue;
        cl::Program::Sources sources;
        cl::Program program;
        cl::Buffer buffer_voxels;
        Volume volume;
        std::clock_t timerStartTicks;

        cl::Image2D clImage_disparity;
};

#endif
