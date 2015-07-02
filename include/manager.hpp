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
                void refreshWindow();

                bool loadNextFrame();
                void getInput();

                Algorithm m_algorithm;

                Image* m_left_rectified = NULL;
                Image* m_right_rectified = NULL;
                Image* m_disparity_map = NULL;
                Image* m_depth_map = NULL;
                Image* m_vertex_map = NULL;
                Image* m_normal_map = NULL;
                Image* m_render = NULL;
                Image* m_output = NULL;

                WindowManager* m_window_manager = NULL;
                bool m_volume_allocated = false;
                int* m_voxels = NULL;
                bool m_more_frames = true;
                unsigned int m_frame_index = 0;
                unsigned int m_volume_size;

                std::string m_footage_directory;
                Util::CameraConfig m_camera_config;

                // Keyboard input
                bool m_done = false;
                bool m_left = false;
                bool m_up = false;
                bool m_right = false;
                bool m_down = false;
                int m_degrees = 0;
                float m_cam_distance = 240;
};

#endif
