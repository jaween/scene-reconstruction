#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "manager.hpp"

// ?? To do: Decouple from SDL input (Use composition? Would that double up on SDL init()?)
#include <SDL2/SDL.h>

Manager::Manager(GraphicsFactory* graphics_factory, std::string footage_directory, Util::CameraConfig& camera_config)
{
        // Loads the rectified images
        left_rectified = graphics_factory->createImage(1, 1, 1);
        right_rectified = graphics_factory->createImage(1, 1, 1);
        more_frames = loadFrame(footage_directory, frame_index);
        frame_index++;

        // Allocates memory for temporary outputs after each pipeline stage
        unsigned int width = left_rectified->getWidth();
        unsigned int height = left_rectified->getHeight();
        disparity_map = graphics_factory->createImage(width, height, 1);
        depth_map = graphics_factory->createImage(width, height, 1);
        vertex_map = graphics_factory->createImageMemory(width, height, 4);
        normal_map = graphics_factory->createImageMemory(width, height, 4);
        render = graphics_factory->createImage(width, height, 1);
        output = disparity_map;

        // Allocates memory for the algorithms
        algorithm.initialise(graphics_factory, width, height);

        // Creates the window for output
        window_manager = graphics_factory->createWindowManager();
        Window::PixelFormat pixel_format = Window::PixelFormat::ABGR;
        std::string title = "Scene Reconstruction";
        window_manager->createWindow(output, pixel_format, title);

        this->footage_directory = footage_directory;
        this->camera_config = camera_config;
}

Manager::~Manager()
{
        delete [] voxels;
}

void Manager::start()
{
        while (!done)
        {
                // Performs the stages of reconstruction
                computeDisparity();
                disparityToDepth();
                trackCamera();
                //fuseIntoVolume();
                //renderVolume();
                refreshOutput();

                // Loads in the next frame for processing
                if (more_frames)
                {
                        more_frames = loadFrame(footage_directory, frame_index);
                        frame_index++;
                }

                // User input
                getInput();
        }
}

void Manager::computeDisparity()
{
        // Generates a disparity map from a stereo pair of images
        int window_size = 9;
        algorithm.generateDisparityMap(left_rectified, right_rectified, window_size, disparity_map);
}

void Manager::disparityToDepth()
{
        // Projects the disparity map into a depth map
        algorithm.convertDisparityMapToDepthMap(disparity_map, camera_config.focal_length, camera_config.baseline, depth_map);
}

void Manager::trackCamera()
{
        // Tracks the camera between frames
        Util::Transformation transformation;
        algorithm.trackCamera(depth_map, vertex_map, normal_map, camera_config, transformation);
}

void Manager::fuseIntoVolume()
{
        // ?? To do: Volumetric integration of depth map into signed distance function 3D representation
        algorithm.tempSetVoxels(disparity_map);
}

void Manager::renderVolume()
{
        // Rendering parameters
        int eye_x = 0;
        int eye_y = 0;
        int eye_z = 340;
        int screen_z = 280;
        float radians = degrees * (M_PI / 180.0);

        // Performs ray tracing on the GPU
        algorithm.render(eye_x, eye_y, eye_z, screen_z, radians, cam_distance, render);
}

void Manager::refreshOutput()
{
        // Refreshes the window
        window_manager->refresh();
        SDL_Delay(16);
}

bool Manager::loadFrame(std::string footage_directory, unsigned int frame_index)
{
        std::cout << "Frame " << frame_index << std::endl;

        std::string extension = ".png";
        std::stringstream filename_stream;

        // Loads the left frame
        filename_stream << footage_directory << "l_" << std::setfill('0') << std::setw(4) << frame_index << extension;
        std::ifstream image_stream = std::ifstream(filename_stream.str().c_str());
        if (!image_stream.good())
        {
                std::cerr << "Could not locate image " << filename_stream.str() << std::endl;
                return false;
        }
        left_rectified->load(filename_stream.str());

        // Loads the right frame
        filename_stream = std::stringstream(std::string());
        filename_stream << footage_directory << "r_" << std::setfill('0') << std::setw(4) << frame_index << extension;
        image_stream = std::ifstream(filename_stream.str().c_str());
        if (!image_stream.good())
        {
                std::cerr << "Could not locate image " << filename_stream.str() << std::endl;
                return false;
        }
        right_rectified->load(filename_stream.str());

        return true;
}

void Manager::getInput()
{
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
                switch (event.type)
                {
                        case SDL_KEYDOWN:
                                left = false;
                                up = false;
                                right = false;
                                down = false;
                                switch (event.key.keysym.sym)
                                {
                                        case SDLK_LEFT:
                                                left = true;
                                                break;
                                        case SDLK_UP:
                                                up = true;
                                                break;
                                        case SDLK_RIGHT:
                                                right = true;
                                                break;
                                        case SDLK_DOWN:
                                                down = true;
                                                break;
                                        case SDLK_ESCAPE:
                                                done = true;
                                                continue;
                                }
                                break;
                        case SDL_KEYUP:
                                switch (event.key.keysym.sym)
                                {
                                        case SDLK_LEFT:
                                                left = false;
                                                break;
                                        case SDLK_UP:
                                                up = false;
                                                break;
                                        case SDLK_RIGHT:
                                                right = false;
                                                break;
                                        case SDLK_DOWN:
                                                down = false;
                                                break;
                                        case SDLK_ESCAPE:
                                                done = false;
                                                continue;
                                }
                                break;
                }
        }

        if (left)
        {
                degrees -= 2;
        }
        if (up)
        {
                cam_distance -= 4;
        }
        if (right)
        {
                degrees += 2;
        }
        if (down)
        {
                cam_distance += 4;
        }
}
