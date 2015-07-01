#include <stdlib.h>
#include <string>

#include "graphics_factory_sdl.hpp"
#include "manager.hpp"
#include "util.hpp"

int main(int argc, char* argv[])
{
        // Footage location
        std::string footage_directory = "res/image/binoculus_2/rectified_";

        // Camera configuration details
        int tsu_baseline_mm = 10;
        int tsu_focal_length = 615;
        int art_baseline_mm = 160;
        int art_focal_length = 3740;
        int bbb_baseline_mm = 119;
        int bbb_focal_length = 145;

        Util::CameraConfig camera_config;
        camera_config.baseline = tsu_baseline_mm;
        camera_config.focal_length = tsu_focal_length;
        camera_config.scale_x = 1;
        camera_config.scale_y = 1;
        camera_config.skew_coeff = 0;

        // Factory to create GUI toolkit specific classes
        GraphicsFactorySdl graphics_factory = GraphicsFactorySdl();

        // Initialises and begins the scene reconstruction pipeline
        Manager manager = Manager(&graphics_factory, footage_directory, camera_config);
        manager.start();

        return EXIT_SUCCESS;
}
