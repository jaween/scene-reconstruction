#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include "image.hpp"

struct Volume
{
        int* voxels;
        int size;
};

class Algorithm
{
    public:
		Algorithm();
		void generateDisparityMap(Image& left, Image& right, unsigned int window_size, Image& disparity);
		void convertDisparityMapToDepthMap(Image& disparity_map, int focal_length, int baseline_mm, Image& depth_map);
                void setVolume(int* voxels, int size);
		void render(int eye_x, int eye_y, int eye_z, int screen_z, float angle, Image& screen);

    private:
		void initialiseOpenCL();
		std::string loadSource(std::string filename);
                Volume volume;
};

#endif
