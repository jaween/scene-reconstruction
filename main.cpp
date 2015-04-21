#include <cmath>
#include <iostream>
#include <string>

#include "algorithm.hpp"
#include "image.hpp"
#include "window_manager.hpp"

void generateVolume(Image& depth_map, int* voxels)
{
    for (int y = 0; y < depth_map.getHeight(); y++)
    {
        for (int x = 0; x < depth_map.getWidth(); x++)
        {
            int pixel_index = y * depth_map.getWidth() + x;
            unsigned int pixel = depth_map.getPixels()[pixel_index];

            int size = depth_map.getWidth();
            int voxel_z = ((pixel & 0xFF) / 255.0) * size;
            int voxel_index = voxel_z * size * size + y * size + x;
            voxels[voxel_index] = pixel & 0xFF;
        }
    }
}

int main()
{    
    Algorithm algorithm = Algorithm();

    // Creates and loads images
    Image l = Image("res/image/tsu_sq_l.png");
    Image r = Image("res/image/tsu_sq_r.png");
    Image disparity_map = Image(l.getWidth(), l.getHeight());
    Image depth_map = Image(l.getWidth(), l.getHeight());
    Image screen = Image(l.getWidth(), l.getHeight());
    screen.fill(0x88);

    // Generates a disparity map from a stereo pair of images
    int window_size = 9;
    std::cout << "Generating disparity map" << std::endl;
    algorithm.generateDisparityMap(l, r, window_size, disparity_map);

    // Projects the disparity map into a depth map
    std::cout << "Converting to depth map" << std::endl;
    int tsukaba_focal_length = 615;
    int tsukaba_baselinemm = 10;
    int art_focal_length = 3740;
    int art_baseline_mm = 160;
    algorithm.convertDisparityMapToDepthMap(disparity_map, art_focal_length, art_baseline_mm, depth_map);

    // Generates a volume and and it to GPU memory
    int size = l.getWidth();
    int* voxels = new int[size * size * size] {};
    generateVolume(depth_map, voxels);
    algorithm.setVolume(voxels, size);

    //Creates the window
    std::string title = "Render";
    WindowManager window_manager = WindowManager();
    Window window = Window(screen, title);
    window_manager.addWindow(&window);

    // Renders the volume in the window
    int eye_x = 0;
    int eye_y = 0;
    int eye_z = 300;
    int screen_z = 280;
    for (int i = 0; i < 360; i += 1)
    {
        float radians = i * (M_PI / 180.0);
        algorithm.render(eye_x, eye_y, eye_z, screen_z, radians, screen);
        window_manager.render();
        SDL_Delay(4);
    }

    delete [] voxels;
}
