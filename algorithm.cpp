#include <fstream>
#include <iostream>
#include <sstream>

#include "algorithm.hpp"

Algorithm::Algorithm()
{
    initialiseOpenCL();
}

void Algorithm::initialiseOpenCL()
{
    // Retrieves the OpenCL platform to be used
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0)
    {
        std::cerr << "No platforms found, check OpencL installation." << std::endl;
        exit(1);
    }
    cl::Platform platform = platforms.at(0);
    std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    // Retrieves the default device of the platform
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    if (devices.size() == 0)
    {
        std::cerr << "No devices found, check OpencL installation." << std::endl;
        exit(1);
    }
    device = devices.at(0);
    std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

    context = cl::Context({device});

    // Loads and builds the kernel
    std::string kernel_code = loadSource("kernels/reconstruction.cl");
    std::cout << "Loaded kernel" << std::endl << "Building kernel..." << std::endl;
    sources.push_back({kernel_code.c_str(), kernel_code.length()});
    program = cl::Program(context, sources);
    if (program.build({device}) != CL_SUCCESS)
    {
        std::cerr << "Error building " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
        exit(1);
    }

    // Command queue
    command_queue = cl::CommandQueue(context, device);
}

void Algorithm::generateDisparityMap(Image* left, Image* right, unsigned int window_size, Image* disparity_map)
{
    startTimer();
    
    // Allocates buffers on the device
    cl::Image2D clImage_left(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), left->getWidth(), left->getHeight(), 0, (void*) left->getPixels());
    cl::Image2D clImage_right(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), right->getWidth(), right->getHeight(), 0, (void*) right->getPixels());
    cl::Image2D clImage_disparity(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), disparity_map->getWidth(), disparity_map->getHeight());
    
    cl::Kernel disparity_kernel(program, "disparity");
    disparity_kernel.setArg(0, clImage_disparity);
    disparity_kernel.setArg(1, clImage_left);
    disparity_kernel.setArg(2, clImage_right);
    disparity_kernel.setArg(3, window_size);

    executeImageKernel(disparity_kernel, clImage_disparity, disparity_map);
    endTimer("Disparity map generation");
}

void Algorithm::convertDisparityMapToDepthMap(Image* disparity_map, int focal_length, int baseline_mm, Image* depth_map)
{
    startTimer();

    cl::Image2D clImage_disparity(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), disparity_map->getWidth(), disparity_map->getHeight(), 0, (void*) disparity_map->getPixels());
    cl::Image2D clImage_depth(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), depth_map->getWidth(), depth_map->getHeight());

    cl::Kernel disparity_kernel(program, "disparityToDepth");
    disparity_kernel.setArg(0, clImage_depth);
    disparity_kernel.setArg(1, clImage_disparity);
    disparity_kernel.setArg(2, focal_length);
    disparity_kernel.setArg(3, baseline_mm);

    executeImageKernel(disparity_kernel, clImage_depth, depth_map);
    endTimer("Depth map projection");
}

void Algorithm::trackCamera(Image* depth_map, Util::CameraConfig* camera_config, Image* vertex_map, Image* normal_map, int* transformation)
{
    startTimer();

    // Vertex map generation
    cl::Image2D clImage_depth(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), depth_map->getWidth(), depth_map->getHeight(), 0, (void*) depth_map->getPixels());
    cl::Image2D clImage_vertex_write(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), vertex_map->getWidth(), vertex_map->getHeight());

    cl::Kernel vertex_kernel(program, "generateVertexMap");
    vertex_kernel.setArg(0, clImage_depth);
    vertex_kernel.setArg(1, camera_config->focal_length);
    vertex_kernel.setArg(2, camera_config->scale_x);
    vertex_kernel.setArg(3, camera_config->scale_y);
    vertex_kernel.setArg(4, camera_config->skew_coeff);
    vertex_kernel.setArg(5, camera_config->principal_point_x);
    vertex_kernel.setArg(6, camera_config->principal_point_y);
    vertex_kernel.setArg(7, clImage_vertex_write);
    
    executeImageKernel(vertex_kernel, clImage_vertex_write, vertex_map);

    // Normal map generation
    cl::Image2D clImage_vertex_read(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), vertex_map->getWidth(), vertex_map->getHeight(), 0, (void*) vertex_map->getPixels());
    cl::Image2D clImage_normal(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), normal_map->getWidth(), normal_map->getHeight());
    cl::Kernel normal_kernel(program, "generateNormalMap");
    normal_kernel.setArg(0, clImage_vertex_read);
    normal_kernel.setArg(1, clImage_normal);

    executeImageKernel(normal_kernel, clImage_normal, normal_map);
    
    endTimer("Tracking camera");
}

void Algorithm::setVolume(int* voxels, int volume_size)
{
    startTimer();
    
    volume.voxels = voxels;
    volume.volume_size = volume_size;
    
    unsigned int voxel_count = volume.volume_size * volume.volume_size * volume.volume_size;
    buffer_voxels = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * voxel_count);
    command_queue.enqueueWriteBuffer(buffer_voxels, CL_TRUE, 0, sizeof(int) * voxel_count, volume.voxels);

    endTimer("Setting volume");
}

void Algorithm::render(int eye_x, int eye_y, int eye_z, int screen_z, float angle, float cam_distance, Image* screen)
{
    cl::Image2D clImage_screen(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), screen->getWidth(), screen->getHeight());

    cl::Kernel disparity_kernel(program, "render");
    disparity_kernel.setArg(0, buffer_voxels);
    disparity_kernel.setArg(1, volume.volume_size);
    disparity_kernel.setArg(2, eye_x);
    disparity_kernel.setArg(3, eye_y);
    disparity_kernel.setArg(4, eye_z);
    disparity_kernel.setArg(5, screen_z);
    disparity_kernel.setArg(6, angle);
    disparity_kernel.setArg(7, cam_distance);
    disparity_kernel.setArg(8, clImage_screen);

    executeImageKernel(disparity_kernel, clImage_screen, screen);
}

std::string Algorithm::loadSource(std::string filename)
{
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

inline void Algorithm::executeImageKernel(cl::Kernel& kernel, cl::Image2D& out_buffer, Image* out_image)
{
    // Enqueues the execution of the kernel
    command_queue.enqueueNDRangeKernel(
        kernel,
        cl::NullRange,
        cl::NDRange(out_image->getWidth(), out_image->getHeight()),
        cl::NullRange
    );

    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = out_image->getWidth();
    region[1] = out_image->getHeight();
    region[2] = 1;
    unsigned int* pixel_data = out_image->getPixels();
    command_queue.enqueueReadImage(out_buffer, CL_TRUE, origin, region, 0, 0, pixel_data, NULL, NULL);
}

void Algorithm::startTimer()
{
    timerStartTicks = std::clock();
}

void Algorithm::endTimer(std::string function)
{
    // Displays the time taken to execute the function
    std::clock_t timerEndTicks = std::clock();
    std::clock_t clockTicksTaken = timerEndTicks - timerStartTicks;
    double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
    double timeInMillis = timeInSeconds * 1000.0;
    //std::cout << function << " took " << timeInMillis  << "ms" << std::endl;
}
