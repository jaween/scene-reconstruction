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
        m_footage_directory = footage_directory;
        m_camera_config = camera_config;

        // Loads the rectified images
        m_left_rectified = graphics_factory->createImage(1, 1, 1);
        m_right_rectified = graphics_factory->createImage(1, 1, 1);
        m_more_frames = loadNextFrame();
        if (!m_more_frames)
        {
                std::cerr << "Could not locate footage in directory " << m_footage_directory << std::endl;
                exit(EXIT_FAILURE);
        }

        // Allocates memory for temporary outputs after each pipeline stage
        unsigned int width = m_left_rectified->getWidth();
        unsigned int height = m_left_rectified->getHeight();
        m_disparity_map = graphics_factory->createImage(width, height, 1);
        m_depth_map = graphics_factory->createImage(width, height, 1);
        m_vertex_map = graphics_factory->createImageMemory(width, height, 4);
        m_normal_map = graphics_factory->createImageMemory(width, height, 4);
        m_render = graphics_factory->createImage(width, height, 1);
        m_output = m_render;

        // Allocates memory for the algorithms
        m_algorithm.initialise(graphics_factory, width, height);

        // Creates the window for output
        m_window_manager = graphics_factory->createWindowManager();
        Window::PixelFormat pixel_format = Window::PixelFormat::ABGR;
        std::string title = "Scene Reconstruction";
        m_window_manager->createWindow(m_output, pixel_format, title);
}

Manager::~Manager()
{
        delete [] m_voxels;
}

void Manager::start()
{
        while (!m_done)
        {
                if (m_more_frames)
                {
                        // Performs the stages of reconstruction
                        computeDisparity();
                        disparityToDepth();
                        trackCamera();
                        fuseIntoVolume();

                        // Loads in the next frame for processing
                        m_more_frames = loadNextFrame();
                }

                // Draws to the screen and gets user input
                renderVolume();
                refreshWindow();
                getInput();
        }
}

void Manager::computeDisparity()
{
        // Generates a disparity map from a stereo pair of images
        const int window_size = 9;
        m_algorithm.generateDisparityMap(m_left_rectified, m_right_rectified, window_size, m_disparity_map);
}

void Manager::disparityToDepth()
{
        // Projects the disparity map into a depth map
        m_algorithm.convertDisparityMapToDepthMap(
                m_disparity_map,
                m_camera_config.focal_length,
                m_camera_config.baseline,
                m_depth_map
        );
}

void Manager::trackCamera()
{
        // Tracks the camera between frames
        Util::Transformation transformation;
        m_algorithm.trackCamera(
                m_depth_map,
                m_vertex_map,
                m_normal_map,
                m_camera_config,
                transformation
        );
}

void Manager::fuseIntoVolume()
{
        // ?? To do: Volumetric integration of depth map into signed distance function 3D representation
        m_algorithm.tempSetVoxels(m_disparity_map);
}

void Manager::renderVolume()
{
        // Rendering parameters
        int eye_x = 0;
        int eye_y = 0;
        int eye_z = 340;
        int screen_z = 280;
        float radians = m_degrees * (M_PI / 180.0);

        // Performs ray tracing on the GPU
        m_algorithm.render(eye_x, eye_y, eye_z, screen_z, radians, m_cam_distance, m_render);
}

void Manager::refreshWindow()
{
        // Updates the output window
        m_window_manager->refresh();
        SDL_Delay(16);
}

bool Manager::loadNextFrame()
{
        std::string extension = ".png";
        std::stringstream filename_stream;

        // Loads the left frame
        filename_stream << m_footage_directory << "l_" << std::setfill('0') << std::setw(4) << m_frame_index << extension;
        std::ifstream image_stream = std::ifstream(filename_stream.str().c_str());
        if (!image_stream.good())
        {
                std::cout << "End of footage" << std::endl;
                return false;
        }
        m_left_rectified->load(filename_stream.str());

        // Loads the right frame
        filename_stream = std::stringstream(std::string());
        filename_stream << m_footage_directory << "r_" << std::setfill('0') << std::setw(4) << m_frame_index << extension;
        image_stream = std::ifstream(filename_stream.str().c_str());
        if (!image_stream.good())
        {
                std::cout << std::endl << "End of footage" << std::endl;
                return false;
        }
        m_right_rectified->load(filename_stream.str());

        std::cout << std::endl << "Frame " << m_frame_index << std::endl;
        m_frame_index++;

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
                                m_left = false;
                                m_up = false;
                                m_right = false;
                                m_down = false;
                                switch (event.key.keysym.sym)
                                {
                                        case SDLK_LEFT:
                                                m_left = true;
                                                break;
                                        case SDLK_UP:
                                                m_up = true;
                                                break;
                                        case SDLK_RIGHT:
                                                m_right = true;
                                                break;
                                        case SDLK_DOWN:
                                                m_down = true;
                                                break;
                                        case SDLK_ESCAPE:
                                                m_done = true;
                                                continue;
                                }
                                break;
                        case SDL_KEYUP:
                                switch (event.key.keysym.sym)
                                {
                                        case SDLK_LEFT:
                                                m_left = false;
                                                break;
                                        case SDLK_UP:
                                                m_up = false;
                                                break;
                                        case SDLK_RIGHT:
                                                m_right = false;
                                                break;
                                        case SDLK_DOWN:
                                                m_down = false;
                                                break;
                                        case SDLK_ESCAPE:
                                                m_done = false;
                                                continue;
                                }
                                break;
                }
        }

        if (m_left)
        {
                m_degrees -= 2;
        }
        if (m_up)
        {
                m_cam_distance -= 4;
        }
        if (m_right)
        {
                m_degrees += 2;
        }
        if (m_down)
        {
                m_cam_distance += 4;
        }
}
