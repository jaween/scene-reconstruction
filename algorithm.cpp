#include <ctime>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <CL/cl.hpp>

#include "algorithm.hpp"

// TODO: Why are there linker errors when trying to include cl.hpp from within algorithm.hpp? 
cl::Device device;
cl::Context context;
cl::CommandQueue command_queue;
cl::Program::Sources sources;
cl::Program program;

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
    std::string kernel_code = loadSource("kernels/disparity.cl");
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

void Algorithm::generateDisparityMap(Image& left, Image& right, unsigned int window_size, Image& disparity)
{    
    std::clock_t startTicks = std::clock();
    
    // Allocates buffers on the device
    cl::Image2D clImage_left(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), left.getWidth(), left.getHeight(), 0, (void*) left.getPixels());
    cl::Image2D clImage_right(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), right.getWidth(), right.getHeight(), 0, (void*) right.getPixels());
    cl::Image2D clImage_disparity(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), disparity.getWidth(), disparity.getHeight());
    
    cl::Kernel disparity_kernel(program, "disparity");
    disparity_kernel.setArg(0, clImage_disparity);
    disparity_kernel.setArg(1, clImage_left);
    disparity_kernel.setArg(2, clImage_right);
    disparity_kernel.setArg(3, window_size);
 
    // Enqueues the execution of the kernel
    command_queue.enqueueNDRangeKernel(
        disparity_kernel,
        cl::NullRange,
        cl::NDRange(left.getWidth(), left.getHeight()),
        cl::NullRange
    );
	
    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = left.getWidth();
    region[1] = left.getHeight();
    region[2] = 1;
    unsigned int* pixel_data = disparity.getPixels();
    command_queue.enqueueReadImage(clImage_disparity, CL_TRUE, origin, region, 0, 0, pixel_data, NULL, NULL);

    // Displays the time taken to execute the function
    std::clock_t endTicks = std::clock();
    std::clock_t clockTicksTaken = endTicks - startTicks;
    double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
    double timeInMillis = timeInSeconds * 1000.0;
    std::cout << "Done! OpenCL disparity map generation took " << timeInMillis  << "ms" << std::endl;
}

void Algorithm::convertDisparityMapToDepthMap(Image& disparity_map, int focal_length, int baseline_mm, Image& depth_map)
{
    std::clock_t startTicks = std::clock();

    cl::Image2D clImage_disparity(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), disparity_map.getWidth(), disparity_map.getHeight(), 0, (void*) disparity_map.getPixels());
    cl::Image2D clImage_depth(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), depth_map.getWidth(), depth_map.getHeight());

    cl::Kernel disparity_kernel(program, "disparityToDepth");
    disparity_kernel.setArg(0, clImage_depth);
    disparity_kernel.setArg(1, clImage_disparity);
    disparity_kernel.setArg(2, focal_length);
    disparity_kernel.setArg(3, baseline_mm);
 
    // Enqueues the execution of the kernel
    command_queue.enqueueNDRangeKernel(
        disparity_kernel,
        cl::NullRange,
        cl::NDRange(disparity_map.getWidth(), disparity_map.getHeight()),
        cl::NullRange
    );

    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = disparity_map.getWidth();
    region[1] = disparity_map.getHeight();
    region[2] = 1;
    unsigned int* pixel_data = depth_map.getPixels();
    command_queue.enqueueReadImage(clImage_depth, CL_TRUE, origin, region, 0, 0, pixel_data, NULL, NULL);
    
    // Displays the time taken to execute the function
    std::clock_t endTicks = std::clock();
    std::clock_t clockTicksTaken = endTicks - startTicks;
    double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
    double timeInMillis = timeInSeconds * 1000.0;
    std::cout << "Done! OpenCL depth map projection took " << timeInMillis  << "ms" << std::endl;
}

void Algorithm::render(int* voxels, int size, Image& screen)
{
    std::clock_t startTicks = std::clock();

    unsigned int voxel_count = size * size * size;
    cl::Buffer buffer_voxels(context, CL_MEM_READ_ONLY, sizeof(int) * voxel_count);
    command_queue.enqueueWriteBuffer(buffer_voxels, CL_TRUE, 0, sizeof(int) * voxel_count, voxels);
    cl::Image2D clImage_screen(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), screen.getWidth(), screen.getHeight());

    cl::Kernel disparity_kernel(program, "render");
    disparity_kernel.setArg(0, buffer_voxels);
    disparity_kernel.setArg(1, size);
    disparity_kernel.setArg(2, clImage_screen);
 
    // Enqueues the execution of the kernel
    command_queue.enqueueNDRangeKernel(
        disparity_kernel,
        cl::NullRange,
        cl::NDRange(screen.getWidth(), screen.getHeight()),
        cl::NullRange
    );

    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = screen.getWidth();
    region[1] = screen.getHeight();
    region[2] = 1;
    unsigned int* pixel_data = screen.getPixels();
    command_queue.enqueueReadImage(clImage_screen, CL_TRUE, origin, region, 0, 0, pixel_data, NULL, NULL);
    
    // Displays the time taken to execute the function
    std::clock_t endTicks = std::clock();
    std::clock_t clockTicksTaken = endTicks - startTicks;
    double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
    double timeInMillis = timeInSeconds * 1000.0;
    std::cout << "Done! OpenCL rendering took " << timeInMillis  << "ms" << std::endl;
}

std::string Algorithm::loadSource(std::string filename)
{
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}
