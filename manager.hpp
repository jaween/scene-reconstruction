#ifndef MANAGER_HPP
#define MANAGER_HPP

#include "algorithm.hpp"
#include "graphics_factory.hpp"
#include "image.hpp"
#include "window_manager.hpp"
#include "util.hpp"

class Manager
{
        public:
                Manager(GraphicsFactory* graphics_factory, std::string footage_directory, Util::CameraConfig& camera_config);
                ~Manager();
                void start();

        private:
                void computeDisparity();
                void disparityToDepth();
                void trackCamera();
                void fuseIntoVolume();
                void renderVolume();
                void refreshOutput();

                bool loadFrame(std::string footage_directory, unsigned int frame_index);   
                void getInput();
                
                Algorithm algorithm;

                Image* left_rectified = NULL;
                Image* right_rectified = NULL;
                Image* disparity_map = NULL;
                Image* depth_map = NULL;
                Image* vertex_map = NULL;
                Image* normal_map = NULL;
                Image* render = NULL;
                Image* output = NULL;
                
                WindowManager* window_manager = NULL;
                bool volume_allocated = false;
                int* voxels = NULL;
                bool more_frames = true;
                unsigned int frame_index = 0;
                unsigned int volume_size;

                std::string footage_directory;
                Util::CameraConfig camera_config;
                
                // Keyboard input
                bool done = false;
                bool left = false;
                bool up = false;
                bool right = false;
                bool down = false;
                int degrees = 0;
                float cam_distance = 240;
};

#endif
