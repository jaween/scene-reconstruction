#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>

#define DEBUG_MODE true

// Utility structures and debug functions
namespace Util
{
        struct CameraConfig
        {
                unsigned int baseline;
                unsigned int focal_length;
                unsigned int principal_point_x;
                unsigned int principal_point_y;
                unsigned int scale_x;
                unsigned int scale_y;
                unsigned int skew_coeff;
        };

        struct Vector3D
        {
                double x;
                double y;
                double z;
        };

        struct Transformation
        {
                Vector3D translation;
                Vector3D rotation;
        };

        void startDebugTimer();
        void endDebugTimer(std::string function);
};

#endif
