const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void disparity(__write_only image2d_t disparity, __read_only image2d_t left, __read_only image2d_t right, const int window_size)
{
    const int width = get_image_width(disparity);
    const int height = get_image_width(disparity);

    // Defines the center of the 'base window' as well as the pixel to be shaded
    int x = get_global_id(0);
    int y = get_global_id(1);

    unsigned int minumum_sum_of_absolute_difference = 1000000000;
    unsigned int disparity_value = 0;

    // Walks the image horizontally, defines the center of the 'moving window'
    for (int window_x = 0; window_x < width; window_x++)
    {
        // Computes the total sum of absolute differences between corresponding pixels in the
        // moving window and the base window
        int running_sum_of_absolute_difference = 0;
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
                running_sum_of_absolute_difference += abs(difference);
            }
        }

        // Keeps track of which movining window was most similar to the base window
        if (running_sum_of_absolute_difference < minumum_sum_of_absolute_difference)
        {
            minumum_sum_of_absolute_difference = running_sum_of_absolute_difference;
            disparity_value = abs(window_x - x);
        }
    }

    // Writes the disparity value to the image (clamped between 0 to 255) 
    disparity_value &= 0xFF;
    uint4 write_pixel = (uint4) (disparity_value);
    write_imageui(disparity, (int2) (x, y), write_pixel);
}

__kernel void disparityToDepth(__write_only image2d_t depth_map, __read_only image2d_t disparity_map, const int focal_length, const int baseline_mm)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    const float baseline = (float) baseline_mm / 1000.0;
    
    // Implements the equation "Z = f * B / d"
    uint disp = read_imageui(disparity_map, sampler, (int2) (x, y)).x;
    uint depth = (int) (((focal_length * baseline) / (float) disp)) & 0xFF;
    uint4 write_pixel = (uint4) (depth);

    write_imageui(depth_map, (int2) (x, y), write_pixel);
}

__kernel void generateVertexMap(__read_only image2d_t depth_map, const int focal_length,
    const int scale_x, const int scale_y, const int skew_coeff, const int principal_point_x,
    const int principal_point_y, __write_only image2d_t vertex_map)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    // Implements depth(x, y) * K_inverse * [x, y, 1]
    int depth = read_imageui(depth_map, sampler, (int2) (x, y)).x;
    float reprojected_x = focal_length * scale_x * x;
    float reprojected_y = skew_coeff * x + focal_length * scale_y * y;
    float reprojected_z = principal_point_x * x + principal_point_y * y + 1;

    unsigned int packed = ((int) (reprojected_x) << 16) | ((int) (reprojected_y) << 8) | (int) (reprojected_z);
    //uint4 vertex = (uint4) (packed);
    uint4 vertex = (uint4) (1, reprojected_x, reprojected_y, reprojected_z);
    vertex *= depth;
    write_imageui(vertex_map, (int2) (get_global_id(0), get_global_id(1)), vertex);
}

__kernel void generateNormalMap(__read_only image2d_t vertex_map, __write_only image2d_t normal_map)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    uint4 center = read_imageui(vertex_map, sampler, (int2) (x, y));
    uint4 right = read_imageui(vertex_map, sampler, (int2) (x + 1, y)) - center;
    uint4 down = read_imageui(vertex_map, sampler, (int2) (x, y + 1)) - center;

    float4 cross_product = (float4) (
                right.y * down.z - right.z * down.y,
                right.z * down.x - right.x  * down.z,
                right.x * down.y - right.y * down.x,
                1);
    cross_product = normalize(cross_product);
    
    uint4 normal = (uint4) (cross_product.x * 255, cross_product.y * 255, cross_product.z * 255, cross_product.w * 255);
    write_imageui(normal_map, (int2) (get_global_id(0), get_global_id(1)), normal);  
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
    if (intersect(origin, dir, (float3) (-volume_size/2, -volume_size/2, -volume_size/2), (float3) (volume_size/2, volume_size/2, volume_size/2), &box_intersection))
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
