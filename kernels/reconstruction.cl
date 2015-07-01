const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void disparity(__write_only image2d_t disparity, __read_only image2d_t left, __read_only image2d_t right, const int window_size)
{
        const int width = get_image_width(disparity);
        const int height = get_image_width(disparity);

        // Defines the center of the 'base window' as well as the pixel to be shaded
        int x = get_global_id(0);
        int y = get_global_id(1);

        unsigned int minumum_sum_of_absolute_differences = 1000000000;
        unsigned int disparity_value = 0;

        // Walks the image horizontally, defines the center of the 'moving window'
        for (int window_x = 0; window_x < width; window_x++)
        {
                // Computes the total sum of absolute differences between corresponding pixels in the
                // moving window and the base window
                int running_sum_of_absolute_differences = 0;
                for (int i = -window_size / 2; i < window_size / 2; i++)
                {
                        for (int j = -window_size / 2; j < window_size / 2; j++)
                        {
                                int2 left_coordinate = (int2) (window_x + i, y + j);
                                int2 right_coordinate = (int2) (x + i, y + j);

                                int left_pixel = read_imageui(left, sampler, left_coordinate).x;
                                int right_pixel = read_imageui(right, sampler, right_coordinate).x;

                                // Performs the SAD computation
                                int difference = left_pixel - right_pixel;
                                running_sum_of_absolute_differences += abs(difference);
                        }
                }

                // Keeps track of which movining window was most similar to the base window
                if (running_sum_of_absolute_differences < minumum_sum_of_absolute_differences)
                {
                        minumum_sum_of_absolute_differences = running_sum_of_absolute_differences;
                        disparity_value = abs(window_x - x);
                }
        }

        // Writes the disparity value to the image (clamped between 0 to 255)
        disparity_value &= 0xFF;
        uint4 write_pixel = (uint4) (disparity_value);
        write_imageui(disparity, (int2) (x, y), write_pixel);
}

// Converts disparity values to depth in millimeters
__kernel void disparityToDepth(__write_only image2d_t depth_map, __read_only image2d_t disparity_map, const int focal_length, const int baseline_mm)
{
        int x = get_global_id(0);
        int y = get_global_id(1);

        // Implements the equation "Z = f * B / d"
        uint disp = read_imageui(disparity_map, sampler, (int2) (x, y)).x;
        uint depth = (focal_length * baseline_mm) / disp;
        uint write_pixel = depth;

        write_imageui(depth_map, (int2) (x, y), write_pixel);
}

__kernel void generateVertexMap(__read_only image2d_t depth_map, const int focal_length,
        const int scale_x, const int scale_y, const int skew_coeff, const int principal_point_x,
        const int principal_point_y, __write_only image2d_t vertex_map)
{
        int2 coord = (int2) (get_global_id(0), get_global_id(1));

        uint depth = read_imageui(depth_map, sampler, coord).x;

        // Implements depth(x, y) * K_inverse * [x, y, 1]
        uint4 vertex = (uint4) (
                (uint) (focal_length * scale_x * coord.x) * depth ,
                (uint) (skew_coeff * coord.x + focal_length * scale_y * coord.y) * depth,
                (uint) (principal_point_x * coord.x + principal_point_y * coord.y + 1) * depth,
                0
        );

        write_imageui(vertex_map, coord, vertex);
}

__kernel void generateNormalMap(__read_only image2d_t vertex_map, __write_only image2d_t normal_map)
{
        int x = get_global_id(0);
        int y = get_global_id(1);

        uint4 center = read_imageui(vertex_map, sampler, (int2) (x, y));
        uint4 right = read_imageui(vertex_map, sampler, (int2) (x + 1, y));
        uint4 down = read_imageui(vertex_map, sampler, (int2) (x, y + 1));

        float4 vector_right = (float4) (right.x - center.x, right.y - center.y, right.z - center.z, 0.0f);
        float4 vector_down = (float4) (down.x - center.x, down.y - center.y, down.z - center.z, 0.0f);

        float4 cross_product = cross(vector_right, vector_down);
        float4 normal = normalize(cross_product);

        write_imagef(normal_map, (int2) (x, y), normal);
}

/**
 * Uses projective data association to find corresponding verticies between the vertex map of
 * the previous frame and the current frame. The initial translation and rotation should be
 * set to the estimated global pose values for the previous frame (i.e. we assume the camera
 * has not moved much between frames). Subsequent pose inputs should be the output of the
 * previous iteration of ICP.
**/
__kernel void findCorrespondences(__read_only image2d_t depth_map,
        __read_only image2d_t prev_vertex_map, __read_only image2d_t prev_normal_map,
        __read_only image2d_t vertex_map, __read_only image2d_t normal_map,
        float translation_x, float translation_y, float translation_z,
        float rot_x, float rot_y, float rot_z, __global float3* correspondences)
{
        // We only find correspondences for coordinates with valid depth
        uint depth = read_imageui(depth_map, sampler, (int2) (get_global_id(0), get_global_id(1))).x;
        if (depth == 0)
        {
                return;
        }

        // Retrieves the vertex from the previous frame
        float3 prev_global_vertex = (float3) ((float) get_global_id(0), (float) get_global_id(1), 0.0f);
        prev_global_vertex.z = (float) (read_imageui(prev_vertex_map, sampler, (int2) (prev_global_vertex.x, prev_global_vertex.y)).x);

        // Transforms this vertex from global coords into camera coords
        // The equations are the result of an inverse 4x4 transformation matrix multiplied with
        // a 4x1 point (ignores the fourth row of the output vector)
        float3 prev_camera_vertex;
        prev_camera_vertex.x =
                prev_global_vertex.x * (cos(rot_z) * cos(rot_y) + sin(rot_x) * sin(rot_y)) +
                prev_global_vertex.y * (-sin(rot_z) * cos(rot_x)) +
                prev_global_vertex.z * (sin(rot_z) * sin(rot_x) * cos(rot_x) - cos(rot_z) * sin(rot_y)) +
                translation_x;
        prev_camera_vertex.y =
                prev_global_vertex.x * (sin(rot_z) * cos(rot_y) - cos(rot_z) * sin(rot_x) * sin(rot_y)) +
                prev_global_vertex.y * (cos(rot_z) * cos(rot_x)) +
                prev_global_vertex.z * (-cos(rot_z) * sin(rot_x) * cos(rot_y) - sin(rot_z) * sin(rot_y)) +
                translation_y;
        prev_camera_vertex.z =
                prev_global_vertex.x * (cos(rot_x) * sin(rot_y)) +
                prev_global_vertex.y * (sin(rot_x)) +
                prev_global_vertex.z * (cos(rot_x) * cos(rot_y)) +
                translation_z;

        // Perspective projects the vector into image coordinates
        // ?? To do: Unsure what viewer vector is and if this is correct. Projection code based on
        // perspective projection section of <https://en.wikipedia.org/?title=3D_projection>
        float3 viewer = (float3) (0.0f, 0.0f, 1.0f);
        int2 prev_image_vertex;
        prev_image_vertex.x = (viewer.z/prev_global_vertex.z) * prev_global_vertex.x - viewer.x;
        prev_image_vertex.y = (viewer.z/prev_global_vertex.z) * prev_global_vertex.y - viewer.y;

        // If prev_image_vertex is within the bounds of the vertex map
        if (prev_image_vertex.x >= 0 && prev_image_vertex.x < get_image_width(depth_map) &&
                prev_image_vertex.y >= 0 && prev_image_vertex.y < get_image_height(depth_map))
        {
                // Retrieves a vertex from the current frame using the perspective projected vector
                float3 camera_vertex = (float3) (prev_image_vertex.x, prev_image_vertex.y, 0.0f);
                camera_vertex.z = (float) (read_imageui(vertex_map, sampler, (int2) ((int) camera_vertex.x, (int) camera_vertex.y)).x);

                // Transforms this vertex from camera coords into global coordinates
                float3 global_vertex;
                global_vertex.x =
                        camera_vertex.x * (cos(rot_z) * cos(rot_y) + sin(rot_x) * sin(rot_y)) +
                        camera_vertex.y * (sin(rot_z) * cos(rot_y) - cos(rot_z) * sin(rot_x) * sin(rot_y)) +
                        camera_vertex.z * (cos(rot_x) * sin(rot_y)) +
                        translation_x;
                global_vertex.y =
                        camera_vertex.x * (-sin(rot_z) * cos(rot_x)) +
                        camera_vertex.y * (cos(rot_z) * cos(rot_x)) +
                        camera_vertex.z * (sin(rot_x)) +
                        translation_y;
                global_vertex.z =
                        camera_vertex.x * (sin(rot_z) * sin(rot_x) * cos(rot_y) - cos(rot_z) * sin(rot_y)) +
                        camera_vertex.y * (-cos(rot_z) * sin(rot_x) * cos(rot_y) - sin(rot_z) * sin(rot_y)) +
                        camera_vertex.z * (cos(rot_x) * cos(rot_y)) +
                        translation_z;

                // Retrieves a normal from the current frame using the perspective projected vector
                float3 camera_normal = (float3) (prev_image_vertex.x, prev_image_vertex.y, 0.0f);
                camera_normal.z = (float) (read_imageui(normal_map, sampler, (int2) ((int) camera_normal.x, (int) camera_normal.y)).x);

                // Rotates this normal using the 3x3 rotation matrix of the
                // camera-coords-to-global-coords transformation
                float3 global_normal;
                global_normal.x =
                        camera_normal.x * (cos(rot_z) * cos(rot_y) + sin(rot_x) * sin(rot_y)) +
                        camera_normal.y * (sin(rot_z) * cos(rot_y) - cos(rot_z) * sin(rot_x) * sin(rot_y)) +
                        camera_normal.z * (cos(rot_x) * sin(rot_y));
                global_normal.y =
                        camera_normal.x * (-sin(rot_z) * cos(rot_x)) +
                        camera_normal.y * (cos(rot_z) * cos(rot_x)) +
                        camera_normal.z * (sin(rot_x));
                global_normal.z =
                        camera_normal.x * (sin(rot_z) * sin(rot_x) * cos(rot_y) - cos(rot_z) * sin(rot_y)) +
                        camera_normal.y * (-cos(rot_z) * sin(rot_x) * cos(rot_y) - sin(rot_z) * sin(rot_y)) +
                        camera_normal.z * (cos(rot_x) * cos(rot_y));

                // ?? To do: Determine best thresholds for camera tracking by varing them
                const uint distance_threshold = 1;
                const float normal_threshold = 1.0f;
                float3 prev_global_normal = (float3) ((float) get_global_id(0), (float) get_global_id(1), 0.0f);
                prev_global_normal.z = (float) (read_imageui(prev_normal_map, sampler, (int2) (prev_global_normal.x, prev_global_normal.y)).x);

                if (length(global_vertex - prev_global_vertex) < distance_threshold &&
                        fabs(dot(global_normal, prev_global_normal)) < normal_threshold)
                {
                        // Correspondence found
                        uint width = get_image_width(depth_map);
                        uint x = get_global_id(0);
                        uint y = get_global_id(1);
                        uint index = y * width + x;
                        correspondences[index] = global_vertex;
                }
        }
}

// Outputs the A matrix and b vector of the equation |Ax + b|, used to compute transformation
__kernel void computeMatricesForTransformation(__read_only image2d_t vertex_map,
        __read_only image2d_t normal_map, __global const float3* correspondences,
        __global float* matrix_a, __global float* vector_b)
{
        uint x = get_global_id(0);
        uint y = get_global_id(1);
        uint width = get_image_width(vertex_map);
        uint index = y * width + x;
        uint row_index = index * 6;

        float3 source = correspondences[index];
        uint4 dest = read_imageui(vertex_map, sampler, (int2) (x, y));
        uint4 normal = read_imageui(normal_map, sampler, (int2) (x, y));

        // ?? To do: Why would I need a tree reduction here?
        // Calculates the elements of a row of matrix A
        matrix_a[row_index + 0] = normal.z * source.y - normal.y * source.z;
        matrix_a[row_index + 1] = normal.x * source.z - normal.z * source.x;
        matrix_a[row_index + 2] = normal.y * source.x - normal.x * source.y;
        matrix_a[row_index + 3] = normal.x;
        matrix_a[row_index + 4] = normal.y;
        matrix_a[row_index + 5] = normal.z;

        // Calculates the componets of the Nx1 vector b
        vector_b[index] = normal.x * dest.x + normal.y * dest.y + normal.z * dest.z
                - normal.x * source.x - normal.y * source.y - normal.z * source.z;
}

bool intersect(float3 ray_origin, float3 ray_direction, float3 box_a, float3 box_b, float3* box_intersection)
{
        float ray_length_enter_box = -1000000000;
        float ray_length_leave_box = 1000000000;

        // Distance from the ray origin to box points along the x axis
        if (ray_direction.x != 0)
        {
                float distance_to_a_along_x = (box_a.x - ray_origin.x) / ray_direction.x;
                float distance_to_b_along_x = (box_b.x - ray_origin.x) / ray_direction.x;
                ray_length_enter_box = min(distance_to_a_along_x, distance_to_b_along_x);
                ray_length_leave_box = max(distance_to_a_along_x, distance_to_b_along_x);
        }

        // Distance from the ray origin to box points along the y axis
        if (ray_direction.y != 0)
        {
                float distance_to_a_along_y = (box_a.y - ray_origin.y) / ray_direction.y;
                float distance_to_b_along_y = (box_b.y - ray_origin.y) / ray_direction.y;
                ray_length_enter_box = max(ray_length_enter_box, min(distance_to_a_along_y, distance_to_b_along_y));
                ray_length_leave_box = min(ray_length_leave_box, max(distance_to_a_along_y, distance_to_b_along_y));
        }

        // Distance from the ray origin to box points along the z axis
        if (ray_direction.z != 0)
        {
                float distance_to_a_along_z = (box_a.z - ray_origin.z) / ray_direction.z;
                float distance_to_b_along_z = (box_b.z - ray_origin.z) / ray_direction.z;
                ray_length_enter_box = max(ray_length_enter_box, min(distance_to_a_along_z, distance_to_b_along_z));
                ray_length_leave_box = min(ray_length_leave_box, max(distance_to_a_along_z, distance_to_b_along_z));
        }

        // Final decision on whether the ray intersected with the box
        if (ray_length_enter_box <= ray_length_leave_box)
        {
                *box_intersection = ray_origin + ray_direction * ray_length_enter_box;
                return true;
        }
        return false;
}

__kernel void render(__global const int* voxels, int volume_size, int eye_x, int eye_y, int eye_z, int screen_z, float angle, float cam_distance, __write_only image2d_t screen)
{
        int screen_width = get_image_dim(screen).x;
        int screen_height = get_image_dim(screen).y;
        int screen_x = get_global_id(0) - screen_width/2;
        int screen_y = screen_height/2 - get_global_id(1);

        float3 eye = (float3) (eye_x, eye_y, eye_z);
        float3 pixel = (float3) (screen_x, screen_y, screen_z);
        float3 dir = normalize(pixel - eye);

        // Ray origin and camera rotation
        float3 origin = normalize(eye);
        float new_origin_x = cam_distance * (origin.x * cos(angle) - origin.z * sin(angle));
        float new_origin_z = cam_distance * (origin.x * sin(angle) + origin.z * cos(angle));
        origin.x = new_origin_x;
        origin.z = new_origin_z;

        float new_dir_x = dir.x * cos(angle) - dir.z * sin(angle);
        float new_dir_z = dir.x * sin(angle) + dir.z * cos(angle);
        dir.x = new_dir_x;
        dir.z = new_dir_z;

        // Determines where the ray intersects with the volume bounding box
        float distance = 0;
        float3 box_intersection = (float3) (0, 0, 0);
	float3 box_a = (float3) (-volume_size/2, -volume_size/2, -volume_size/2);
	float3 box_b = (float3) (volume_size/2, volume_size/2, volume_size/2);
        if (intersect(origin, dir, box_a, box_b, &box_intersection))
        {
                // Walks the ray through the volume until it hits a non-zero voxel
                for (int i= 0; i < volume_size; i++)
                {
                        unsigned int voxel_coord_x = (int) (box_intersection.x + dir.x * i + volume_size/2);
                        unsigned int voxel_coord_y = volume_size - (int) (box_intersection.y + dir.y * i + volume_size/2);
                        unsigned int voxel_coord_z = volume_size - (int) (box_intersection.z + dir.z * i + volume_size/2);
                        if (voxel_coord_x > volume_size || voxel_coord_y > volume_size || voxel_coord_z > volume_size)
                        {
                                distance = 0;
                        }
                        else
                        {
                                unsigned int index = voxel_coord_z * volume_size * volume_size + voxel_coord_y * volume_size + voxel_coord_x;
                                int voxel = voxels[index];
                                if (voxel != 0)
                                {
                                        distance = 255 - voxel;//(1 - length(origin - box_intersection) / 300) * 255;
                                        break;
                                }
                        }
                }
                // Debug cube
                //distance = (1 - length(origin - box_intersection) / 300) * 255;//255 - (length(origin - box_intersection))/2;
        }

        // Draws the pixel based on the voxel's depth (black if ray did not intersect bounding box)
        uint4 write_pixel = (uint4) (distance);
        write_imageui(screen, (int2) (get_global_id(0), get_global_id(1)), write_pixel);
}
