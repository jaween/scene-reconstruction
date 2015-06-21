#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "manager.hpp"

Manager::Manager(std::string footage_directory, Util::CameraConfig& camera_config)
{
    // Loads the rectified images
    left_rectified = new Image();
    right_rectified = new Image();
    more_frames = loadFrame(footage_directory, frame_index);
    frame_index++;

    // Allocates memory for temporary outputs after each pipeline stage
    disparity_map = new Image(left_rectified->getWidth(), left_rectified->getHeight());
    depth_map = new Image(left_rectified->getWidth(), left_rectified->getHeight());
    vertex_map = new Image(left_rectified->getWidth(), left_rectified->getHeight());
    normal_map = new Image(left_rectified->getWidth(), left_rectified->getHeight());
    render = new Image(left_rectified->getWidth(), left_rectified->getHeight());
    output = disparity_map;

    // Creates the window for output
    Window::PixelFormat pixel_format = Window::PixelFormat::ABGR;
    std::string title = "Scene Reconstruction";
    window_manager.createWindow(output, pixel_format, title);

    this->footage_directory = footage_directory;
    this->camera_config = camera_config;
}

Manager::~Manager()
{
    delete left_rectified;
    delete right_rectified;
    delete disparity_map;
    delete depth_map;
    delete vertex_map;
    delete normal_map;
    delete render;
    
    if (voxels != NULL)
    {
        delete [] voxels;
    }
}

void Manager::start()
{
    while (!done)
    {
        // Performs the stages of reconstruction
        computeDisparity();
        disparityToDepth();
        trackCamera();
        fuseIntoVolume();
        renderVolume();

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
    //algorithm.trackCamera(depth_map, &intrinsics, vertex_map, normal_map, NULL);
}

void Manager::fuseIntoVolume()
{
    // Generates a volume and and it to GPU memory
    if (!volume_allocated)
    {
        int volume_size = std::max(depth_map->getWidth(), depth_map->getHeight());
        generateVolume(volume_size);
        algorithm.setVolume(voxels, volume_size);
        
        volume_allocated = true;
    }
}

void Manager::generateVolume(int volume_size)
{
    voxels = new int[volume_size * volume_size * volume_size] {};
    for (int y = 0; y < depth_map->getHeight(); y++)
    {
        for (int x = 0; x < depth_map->getWidth(); x++)
        {
            int pixel_index = y * depth_map->getWidth() + x;
            unsigned int pixel = depth_map->getPixels()[pixel_index];

            int volume_size = depth_map->getWidth();
            int voxel_z = ((pixel & 0xFF) / 255.0) * volume_size;
            int voxel_index = voxel_z * volume_size * volume_size + y * volume_size + x;
            voxels[voxel_index] = pixel & 0xFF;
        }
    }
}

void Manager::renderVolume()
{
    // Rendering parameters
    int eye_x = 0;
    int eye_y = 0;
    int eye_z = 340;
    int screen_z = 280;

    // Renders the volume in the window
    float radians = degrees * (M_PI / 180.0);
    algorithm.render(eye_x, eye_y, eye_z, screen_z, radians, cam_distance, render);
    window_manager.render();
    SDL_Delay(16);
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
