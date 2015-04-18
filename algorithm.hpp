#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include "image.hpp"

class Algorithm
{
    public:
		Algorithm();
		void generateDisparityMap(Image& left, Image& right, unsigned int window_size, Image& disparity);
		void convertDisparityMapToDepthMap(Image& disparity_map, int focal_length, int baseline_mm, Image& depth_map);
		void render(int* voxels, int size, Image& screen);

    private:
		void initialiseOpenCL();
		std::string loadSource(std::string filename);
};

#endif
