#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

#include "algorithm.hpp"

Algorithm::Algorithm()
{
        initialiseOpenCL();
}

Algorithm::~Algorithm()
{
        // ?? Temp: CPU voxel storage
        delete [] volume.voxels;
}


void Algorithm::initialise(GraphicsFactory* graphics_factory, unsigned int image_width, unsigned int image_height)
{
        // ?? Temp: Allocates memory on the CPU
        unsigned int cube_width = std::max(image_width, image_height);
        unsigned int voxel_count = cube_width * cube_width * cube_width;
        volume.voxels = new int[voxel_count];
        volume.cube_width = cube_width;

        // Allocates a buffer on the GPU for the volume
        buffer_voxels = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * voxel_count);
        command_queue.enqueueWriteBuffer(buffer_voxels, CL_TRUE, 0, sizeof(int) * voxel_count, volume.voxels);

        prev_normal_map = graphics_factory->createImageMemory(image_width, image_height, 4);
        prev_vertex_map = graphics_factory->createImageMemory(image_width, image_height, 4);
}

void Algorithm::initialiseOpenCL()
{
        // Retrieves the OpenCL platform to be used
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.size() == 0)
        {
                std::cerr << "No platforms found, check OpencL installation." << std::endl;
                exit(EXIT_FAILURE);
        }
        cl::Platform platform = platforms.at(0);
        std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

        // Retrieves the default device of the platform
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
        if (devices.size() == 0)
        {
                std::cerr << "No devices found, check OpencL installation." << std::endl;
                exit(EXIT_FAILURE);
        }
        device = devices.at(0);
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

        context = cl::Context({device});

        // Loads and builds the kernel
        std::string kernel_code = loadSource("src/kernels/reconstruction.cl");
        std::cout << "Loaded kernel" << std::endl << "Building kernel..." << std::endl;
        sources.push_back({kernel_code.c_str(), kernel_code.length()});
        program = cl::Program(context, sources);
        if (program.build({device}) != CL_SUCCESS)
        {
                std::cerr << "Error building " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
                exit(EXIT_FAILURE);
        }
        else
        {
                std::cout << "Success!" << std::endl;
        }

        // Command queue
        command_queue = cl::CommandQueue(context, device);
}

void Algorithm::generateDisparityMap(Image* left, Image* right, unsigned int window_size, Image* disparity_map)
{
        Util::startDebugTimer("Disparity map");

        // Allocates buffers on the device
        cl::Image2D clImage_left(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), left->getWidth(), left->getHeight(), 0, (void*) left->getPixels());
        cl::Image2D clImage_right(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), right->getWidth(), right->getHeight(), 0, (void*) right->getPixels());
        cl::Image2D clImage_disparity(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), disparity_map->getWidth(), disparity_map->getHeight());

        cl::Kernel reconstruction_kernel(program, "disparity");
        reconstruction_kernel.setArg(0, clImage_disparity);
        reconstruction_kernel.setArg(1, clImage_left);
        reconstruction_kernel.setArg(2, clImage_right);
        reconstruction_kernel.setArg(3, window_size);

        executeImageKernel(reconstruction_kernel, clImage_disparity, disparity_map);

        Util::endDebugTimer("Disparity map");
}

void Algorithm::convertDisparityMapToDepthMap(Image* disparity_map, int focal_length, int baseline_mm, Image* depth_map)
{
        Util::startDebugTimer("Depth map");

        cl::Image2D clImage_disparity(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), disparity_map->getWidth(), disparity_map->getHeight(), 0, (void*) disparity_map->getPixels());
        cl::Image2D clImage_depth(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_R, CL_UNSIGNED_INT32), depth_map->getWidth(), depth_map->getHeight());

        cl::Kernel reconstruction_kernel(program, "disparityToDepth");
        reconstruction_kernel.setArg(0, clImage_depth);
        reconstruction_kernel.setArg(1, clImage_disparity);
        reconstruction_kernel.setArg(2, focal_length);
        reconstruction_kernel.setArg(3, baseline_mm);

        executeImageKernel(reconstruction_kernel, clImage_depth, depth_map);

        Util::endDebugTimer("Depth map");
}

void Algorithm::trackCamera(Image* depth_map, Image* vertex_map, Image* normal_map, const Util::CameraConfig& camera_config, const Util::Transformation& transformation)
{
        // Vertex map generation
        Util::startDebugTimer("Vertex map");

        cl::Image2D clImage_depth(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_R, CL_UNSIGNED_INT32), depth_map->getWidth(), depth_map->getHeight(), 0, (void*) depth_map->getPixels());
        cl::Image2D clImage_vertex_write(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT32), vertex_map->getWidth(), vertex_map->getHeight());

        cl::Kernel vertex_kernel(program, "generateVertexMap");
        vertex_kernel.setArg(0, clImage_depth);
        vertex_kernel.setArg(1, camera_config.focal_length);
        vertex_kernel.setArg(2, camera_config.scale_x);
        vertex_kernel.setArg(3, camera_config.scale_y);
        vertex_kernel.setArg(4, camera_config.skew_coeff);
        vertex_kernel.setArg(5, camera_config.principal_point_x);
        vertex_kernel.setArg(6, camera_config.principal_point_y);
        vertex_kernel.setArg(7, clImage_vertex_write);

        executeImageKernel(vertex_kernel, clImage_vertex_write, vertex_map);
        Util::endDebugTimer("Vertex map");

        // Normal map generation
        Util::startDebugTimer("Normal map");
        cl::Image2D clImage_vertex_read(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT32), vertex_map->getWidth(), vertex_map->getHeight(), 0, (void*) vertex_map->getPixels());
        cl::Image2D clImage_normal(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_FLOAT), normal_map->getWidth(), normal_map->getHeight());

        cl::Kernel normal_kernel(program, "generateNormalMap");
        normal_kernel.setArg(0, clImage_vertex_read);
        normal_kernel.setArg(1, clImage_normal);

        executeImageKernel(normal_kernel, clImage_normal, normal_map);
        Util::endDebugTimer("Normal map");

        // Determines point correspondences to the previous frame
        Util::startDebugTimer("Correspondences");

        cl::Image2D clImage_prev_vertex(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT32), prev_vertex_map->getWidth(), prev_vertex_map->getHeight(), 0, (void*) prev_vertex_map->getPixels());
        cl::Image2D clImage_prev_normal(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), prev_normal_map->getWidth(), prev_normal_map->getHeight(), 0, (void*) prev_normal_map->getPixels());
        cl::Image2D clImage_normal_read(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), normal_map->getWidth(), normal_map->getHeight(), 0, (void*) normal_map->getPixels());
        unsigned int pixel_count = vertex_map->getWidth() * vertex_map->getHeight();
        clBuffer_correspondences = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * pixel_count);

        cl::Kernel correspondences_kernel(program, "findCorrespondences");
        correspondences_kernel.setArg(0, clImage_depth);
        correspondences_kernel.setArg(1, clImage_prev_vertex);
        correspondences_kernel.setArg(2, clImage_prev_normal);
        correspondences_kernel.setArg(3, clImage_vertex_read);
        correspondences_kernel.setArg(4, clImage_normal_read);
        correspondences_kernel.setArg(5, transformation.translation.x);
        correspondences_kernel.setArg(6, transformation.translation.y);
        correspondences_kernel.setArg(7, transformation.translation.z);
        correspondences_kernel.setArg(8, transformation.rotation.x);
        correspondences_kernel.setArg(9, transformation.rotation.x);
        correspondences_kernel.setArg(10, transformation.rotation.x);
        correspondences_kernel.setArg(11, clBuffer_correspondences);

        executeImageKernel(correspondences_kernel, clImage_normal, normal_map);

        // Keeps a copy of these vertex and normal maps for the next frame
        size_t size_in_bytes = vertex_map->getWidth() * vertex_map->getHeight() *
                vertex_map->getWordsPerPixel() * sizeof(uint32_t);
        memcpy(prev_vertex_map->getPixels(), vertex_map->getPixels(), size_in_bytes);
        memcpy(prev_normal_map->getPixels(), normal_map->getPixels(), size_in_bytes);

        Util::endDebugTimer("Correspondences");
}

// ?? Temp: Pushing the entire volume to the GPU each frame
void Algorithm::tempSetVoxels(Image* image)
{
        // Updates the CPU volume
        Util::startDebugTimer("Set CPU voxels");
        for (int y = 0; y < image->getHeight(); y++)
        {
                for (int x = 0; x < image->getWidth(); x++)
                {
                        int pixel_index = y * image->getWidth() + x;
                        unsigned int pixel = ((float)image->getPixels()[pixel_index] / 300.0f) * 244 ;

                        int volume_width = std::max(image->getWidth(), image->getHeight());
                        int voxel_z = ((pixel & 0xFF) / 255.0) * volume_width;
                        int voxel_index = voxel_z * volume_width * volume_width + y * volume_width + x;
                        volume.voxels[voxel_index] = pixel & 0xFF;
                }
        }
        Util::endDebugTimer("Set CPU voxels");

        // Pushes the volume to the GPU
        Util::startDebugTimer("Push voxels");
        unsigned int voxel_count = image->getWidth() * image->getWidth() * image->getWidth();
        command_queue.enqueueWriteBuffer(buffer_voxels, CL_TRUE, 0, sizeof(int) * voxel_count, volume.voxels);
        Util::endDebugTimer("Push voxels");

}

void Algorithm::render(int eye_x, int eye_y, int eye_z, int screen_z, float angle, float cam_distance, Image* screen)
{
        cl::Image2D clImage_screen(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), screen->getWidth(), screen->getHeight());

        cl::Kernel reconstruction_kernel(program, "render");
        reconstruction_kernel.setArg(0, buffer_voxels);
        reconstruction_kernel.setArg(1, volume.cube_width);
        reconstruction_kernel.setArg(2, eye_x);
        reconstruction_kernel.setArg(3, eye_y);
        reconstruction_kernel.setArg(4, eye_z);
        reconstruction_kernel.setArg(5, screen_z);
        reconstruction_kernel.setArg(6, angle);
        reconstruction_kernel.setArg(7, cam_distance);
        reconstruction_kernel.setArg(8, clImage_screen);

        executeImageKernel(reconstruction_kernel, clImage_screen, screen);
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

        // Offset from which to begin reading
        cl::size_t<3> origin;
        origin[0] = 0;
        origin[1] = 0;
        origin[2] = 0;

        // Rectangle of data to be read
        cl::size_t<3> region;
        region[0] = out_image->getWidth();
        region[1] = out_image->getHeight();
        region[2] = 1;

        uint32_t* pixel_data = out_image->getPixels();
        command_queue.enqueueReadImage(out_buffer, CL_TRUE, origin, region, 0, 0, pixel_data, NULL, NULL);
}
