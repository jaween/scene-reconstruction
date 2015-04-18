const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void disparity(__write_only image2d_t disparity, __read_only image2d_t left, __read_only image2d_t right, const int window_size)
{
    const int width = get_image_width(disparity);
    const int height = get_image_width(disparity);
    const int x = get_global_id(0);
    const int y = get_global_id(1);

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
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const float baseline = (float) baseline_mm / 1000.0;
    
    // Implements the equation "Z = f * B / d"
    uint disp = read_imageui(disparity_map, sampler, (int2) (x, y)).x;
    uint depth = (int) (((focal_length * baseline) / (float) disp)) & 0xFF;
    uint4 write_pixel = (uint4) (depth);

    write_imageui(depth_map, (int2) (x, y), write_pixel);
}

__kernel void depthToVertex(__read_only image2d_t depth_map, __global const int* inverse_intrinsic_matrix, __global const int* vertex_martrix)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    
}

/*bool intersect(float3 ray_origin, float3 ray_direction, int3 box_point_a, int3 box_point_b, int3* box_intersection)
{
    float ray_length_to_box_point_a;
    float ray_length_to_box_point_b;
    
    float box_point_a_intersect_x = (box_point_a.x - ray_origin.x) / ray_direction.x;
    float box_point_a_intersect_y = (box_point_a.y - ray_origin.y) / ray_direction.y;
    ray_length_to_box_point_a = (box_point_a_intersect_x > box_point_a_intersect_y) ? box_point_a_intersect_x : box_point_a_intersect_y;
    
    
    float box_point_b_intersect_y = (box_point_b.y - ray_origin.y) / ray_direction.y;
    float box_point_b_intersect_x = (box_point_b.x - ray_origin.x) / ray_direction.x;
    ray_length_to_box_point_b = (box_point_b_intersect_x > box_point_b_intersect_y) ? box_point_b_intersect_x : box_point_b_intersect_y;
    if (box_point_a_intersect_x > box_point_b_intersect_y || box_point_a_intersect_y > box_point_b_intersect_x)
    {
        return false;
    }
    
    float box_point_a_intersect_z = (box_point_a.z - ray_origin.z) / ray_direction.z;
    float box_point_b_intersect_z = (box_point_b.z - ray_origin.z) / ray_direction.z;
    if (ray_length_to_box_point_a > box_point_b_intersect_z || box_point_a_intersect_z > ray_length_to_box_point_b)
    {
        return false;
    }
    if (box_point_a_intersect_z > ray_length_to_box_point_a)
    {
        ray_length_to_box_point_a = box_point_a_intersect_z;
    }
    if (box_point_b_intersect_z < ray_length_to_box_point_b)
    {
        ray_length_to_box_point_b = box_point_b_intersect_z;
    }

    float ray_length = min(ray_length_to_box_point_a, ray_length_to_box_point_b);
    box_intersection->x = ray_length * ray_direction.x;
    box_intersection->y = ray_length * ray_direction.y;
    box_intersection->z = ray_length * ray_direction.z;
    return true;
}*/
bool intersect(float3 ray_origin, float3 ray_direction, float3 min, float3 max, float3* box_intersection)
{
    float tmin = (min.x - ray_origin.x) / ray_direction.x;
    float tmax = (max.x - ray_origin.x) / ray_direction.x;
    if (tmin > tmax)
    {
        float temp = tmin;
        tmin = tmax;
        tmax = temp;
    }
    float tymin = (min.y - ray_origin.y) / ray_direction.y;
    float tymax = (max.y - ray_origin.y) / ray_direction.y;
    if (tymin > tymax)
    {
        float temp = tymin;
        tymin = tymax;
        tymax = temp;
    }
    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;
    float tzmin = (min.z - ray_origin.z) / ray_direction.z;
    float tzmax = (max.z - ray_origin.z) / ray_direction.z;
    if (tzmin > tzmax)
    {
        float temp = tzmin;
        tzmin = tzmax;
        tzmax = temp;
    }
    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;
    //if ((tmin > r.tmax) || (tmax < r.tmin)) return false;
    //if (r.tmin < tmin) r.tmin = tmin;
    *box_intersection = ray_direction * tmin;
    //if (r.tmax > tmax) r.tmax = tmax;
    return true;
}

__kernel void render(__global const int* voxels, int size, __write_only image2d_t screen)
{    
    const float3 eye = (float3) (0, 0, 10);
    const float screen_z = 5;
    const float3 pixel = (float3) (get_global_id(0), get_global_id(1), screen_z);
    float3 dir = pixel - eye;
    dir = normalize(dir);

    // Determines where the ray intersects with the voxel bounding box
    int distance = 0x00;
    float3 box_intersection;

    for (int i = 0; i < size; i++)
    {
        int index = i * size * size + get_global_id(1) * size + get_global_id(0);
        int voxel = voxels[index];
        if (voxel != 0)
            distance = voxel;
        //if (distance > 255) distance = 0;
    }
    
    /*if (intersect(eye, dir, (float3) (0,0,0), (float3) (size, size, size), &box_intersection))
    {
        // Walks the ray through the data until it hits a non-zero voxel
        int3 pixel_intersection = box_intersection;
        for (distance = 0; distance < size; distance++)
        {
            pixel_intersection.x = (int) (box_intersection.x + dir.x*distance);
            pixel_intersection.y = (int) (box_intersection.y + dir.y*distance);
            pixel_intersection.z = (int) (box_intersection.z + dir.z*distance);
            int index = pixel_intersection.z * size * size + pixel_intersection.y * size + pixel_intersection.x;
            int voxel = voxels[index];
            if (voxel != 0)
            {
                break;
            }
        }
        distance = 0x00;
    }*/

    // Draws the pixel based on the voxel's depth (black if ray did not intersect bounding box)
    uint4 write_pixel = (uint4) (distance);
    write_imageui(screen, (int2) (get_global_id(0), get_global_id(1)), write_pixel);
}
