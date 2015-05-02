#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "algorithm.hpp"
#include "image.hpp"
#include "window_manager.hpp"

void generateVolume(Image& depth_map, int* voxels, int volume_size)
{
    for (int y = 0; y < depth_map.getHeight(); y++)
    {
        for (int x = 0; x < depth_map.getWidth(); x++)
        {
            int pixel_index = y * depth_map.getWidth() + x;
            unsigned int pixel = depth_map.getPixels()[pixel_index];

            int volume_size = depth_map.getWidth();
            int voxel_z = ((pixel & 0xFF) / 255.0) * volume_size;
            int voxel_index = voxel_z * volume_size * volume_size + y * volume_size + x;
            voxels[voxel_index] = pixel & 0xFF;
        }
    }
}

void tempManageInput(bool& done, bool& left, bool& up, bool& right, bool& down, int& degrees, float& cam_distance)
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

int main()
{
    Algorithm algorithm = Algorithm();

    // Creates and loads images
    Image l = Image("res/image/tsu_l.png");
    Image r = Image("res/image/tsu_r.png");
    Image disparity_map = Image(l.getWidth(), l.getHeight());
    Image depth_map = Image(l.getWidth(), l.getHeight());
    Image vertex_map = Image(l.getWidth(), l.getHeight());
    Image normal_map = Image(l.getWidth(), l.getHeight());
    Image screen = Image(l.getWidth(), r.getHeight());

    // Generates a disparity map from a stereo pair of images
    int window_size = 9;
    std::cout << "Generating disparity map" << std::endl;
    algorithm.generateDisparityMap(l, r, window_size, disparity_map);

    // Projects the disparity map into a depth map
    std::cout << "Converting to depth map" << std::endl;
    int tsu_focal_length = 615;
    int tsu_baseline_mm = 10;
    int art_focal_length = 3740;
    int art_baseline_mm = 160;
    algorithm.convertDisparityMapToDepthMap(disparity_map, art_focal_length, art_baseline_mm, depth_map);
    //depth_map = Image("res/image/depth_test.png");

    // Tracks the camera between frames
    camera_intrinsics intrinsics;
    intrinsics.focal_length = tsu_focal_length;
    intrinsics.scale_x = 1;
    intrinsics.scale_y = 1;
    intrinsics.skew_coeff = 0;
    algorithm.trackCamera(depth_map, &intrinsics, vertex_map, normal_map, NULL);

    // Generates a volume and and it to GPU memory
    int volume_size = std::max(depth_map.getWidth(), depth_map.getHeight());
    int* voxels = new int[volume_size * volume_size * volume_size] {};
    generateVolume(depth_map, voxels, volume_size);
    algorithm.setVolume(voxels, volume_size);

    // Initialises the window
    std::string title = "Render";
    WindowManager window_manager = WindowManager();
    Window window = Window(screen, title);
    window_manager.addWindow(&window);

    // Rendering parameters
    int eye_x = 0;
    int eye_y = 0;
    int eye_z = 340;
    int screen_z = 280;
    int degrees = 0;
    float cam_distance = 240;

    // Temp keyboard input
    bool left = true;
    bool up = false;
    bool right = false;
    bool down = false;

    bool done = false;
    while (!done)
    {
        // Retrieves user input
        tempManageInput(done, left, up, right, down, degrees, cam_distance);
        
        // Renders the volume in the window
        float radians = degrees * (M_PI / 180.0);
        algorithm.render(eye_x, eye_y, eye_z, screen_z, radians, cam_distance, screen);
        window_manager.render();
        SDL_Delay(4);
    }

    delete [] voxels;
}
