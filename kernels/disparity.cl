const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void disparity(__write_only image2d_t disparity, __read_only image2d_t left, __read_only image2d_t right, const int window_size)
{
    const int width = get_image_width(disparity);
    const int height = get_image_width(disparity);
    int x = get_global_id(0);
    int y = get_global_id(1);

    unsigned int minimum_sad = 1000000000;
	unsigned int disparity_value = 0;

    for (int window_x = 0; window_x < width; window_x++)
    {
        int running_intensity = 0;
        for (int i = -window_size / 2; i < window_size / 2; i++)
        {
            for (int j = -window_size / 2; j < window_size / 2; j++)
            {
                int2 left_coordinate = (int2) (window_x + i, y + j);
                int2 right_coordinate = (int2) (x + i, y + j);
                
                int left_pixel = read_imageui(left, sampler, left_coordinate).x;
                int right_pixel = read_imageui(right, sampler, right_coordinate).x;
                
                // Sum of absolute of differences
                int diff = left_pixel - right_pixel;
                running_intensity += abs(diff);
            }
        }

        // Keeps the minimum
        if (running_intensity < minimum_sad)
        {
            minimum_sad = running_intensity;
            disparity_value = abs(window_x - x);
        }
    }

    // Writes the disparity value to the image
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

__kernel void depthToVertex(__read_only image2d_t depth_map, __global const int* inverse_intrinsic_matrix, __global const int* vertex_martrix)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    
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

__kernel void render(__global const int* voxels, int size, int eye_x, int eye_y, int eye_z, int screen_z, float angle, __write_only image2d_t screen)
{
    int screen_x = get_global_id(0) - size/2;
    int screen_y = size/2 - get_global_id(1);
    float3 eye = (float3) (eye_x, eye_y, eye_z);
    float3 pixel = (float3) (screen_x, screen_y, screen_z);
    float3 dir = normalize(pixel - eye);

    // Ray origin and camera rotation
    float3 origin = normalize(eye);
    float radius = 230;
    float new_origin_x = radius * (origin.x * cos(angle) - origin.z * sin(angle));
    float new_origin_z = radius * (origin.x * sin(angle) + origin.z * cos(angle));
    origin.x = new_origin_x;
    origin.z = new_origin_z;

    float new_dir_x = dir.x * cos(angle) - dir.z * sin(angle);
    float new_dir_z = dir.x * sin(angle) + dir.z * cos(angle);
    dir.x = new_dir_x;
    dir.z = new_dir_z;

    // Determines where the ray intersects with the volume bounding box
    float distance = 0;
    float3 box_intersection = (float3) (0, 0, 0);
    if (intersect(origin, dir, (float3) (-size/2, -size/2, -size/2), (float3) (size/2, size/2, size/2), &box_intersection))
    {
        // Walks the ray through the data until it hits a non-zero voxel
        for (int i= 0; i < size; i++)
        {
            int voxel_coord_x = (int) (box_intersection.x + dir.x * i + size/2);
            int voxel_coord_y = (int) (box_intersection.y + dir.y * i + size/2);
            int voxel_coord_z = (int) (box_intersection.z + dir.z * i + size/2);
            int index = voxel_coord_z * size * size + voxel_coord_y * size + voxel_coord_x;
            int voxel = voxels[index];
            if (voxel != 0)
            {
                distance = voxel;
                break;
            }
        }
        distance = (1 - length(origin - box_intersection) / 500) * 255;//255 - (length(origin - box_intersection))/2;
    }

    // Draws the pixel based on the voxel's depth (black if ray did not intersect bounding box)
    uint4 write_pixel = (uint4) (distance);
    write_imageui(screen, (int2) (get_global_id(0), get_global_id(1)), write_pixel);
}
