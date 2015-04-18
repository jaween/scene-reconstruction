#include <iostream>
#include <string>

#include "algorithm.hpp"
#include "image.hpp"
#include "window_manager.hpp"

void setVoxels(Image& depth_map, int* voxels)
{
    for (int y = 0; y < depth_map.getHeight(); y++)
    {
        for (int x = 0; x < depth_map.getWidth(); x++)
        {
            int pixel_index = y * depth_map.getWidth();
            unsigned int pixel = depth_map.getPixels()[pixel_index];

            int size = depth_map.getWidth();
            int voxel_z = ((float) pixel / 255.0) * size;
            int voxel_index = voxel_z * size * size + y * size + x;
            voxels[voxel_index] = pixel;
        }
    }
}

int main()
{    
    Algorithm algorithm = Algorithm();
    
    Image l = Image("res/image/tsu_sq_l.png");
    Image r = Image("res/image/tsu_sq_r.png");
    Image disparity_map = Image(l.getWidth(), l.getHeight());
    Image depth_map = Image(l.getWidth(), l.getHeight());
    Image screen = Image(l.getWidth(), l.getHeight());
    screen.fill(0x33);

    int window_size = 9;
    std::cout << "Generating disparity map" << std::endl;
    algorithm.generateDisparityMap(l, r, window_size, disparity_map);

    std::cout << "Converting to depth map" << std::endl;
    int tsukaba_focal_length = 615;
    int tsukaba_baselinemm = 10;
    int art_focal_length = 3740;
    int art_baseline_mm = 160;
    algorithm.convertDisparityMapToDepthMap(disparity_map, art_focal_length, art_baseline_mm, depth_map);

    int size = l.getWidth();
    int* voxels = new int[size * size * size] {};
    algorithm.render(voxels, size, screen);
    
    std::string title = "Render";
    WindowManager window_manager = WindowManager();
    Window window = Window(screen, title);
    window_manager.addWindow(&window);
    
    window_manager.render();
    SDL_Delay(2000);

    delete [] voxels;
}
