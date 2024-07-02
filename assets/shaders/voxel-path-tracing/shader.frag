#version 450 core

layout(location = 0) in vec2 in_pos;  // pos on the screen (-1, 1).

layout(location = 0) out vec4 out_color;  // color of the pixel.

layout(std140, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
}
camera;

struct Voxel {
    bool occupied;
    vec3 color;
};

struct Ray {
    vec3 pos;
    vec3 direction;
    Voxel hit_voxel;
    bool hit;
};

layout(std140, binding = 0) buffer Voxels {
    int width_x;
    int width_y;
    int width_z;
    Voxel voxels[];
}
voxels;

Voxel get_voxel(int x, int y, int z) {
    return voxels.voxels[x + y * voxels.width_x + z * voxels.width_x * voxels.width_y];
}

Ray get_ray(vec2 pos) {
    vec3 direction = normalize(vec3(pos, 1.0));
    return Ray(-camera.view[3].xyz, (camera.projection * vec4(direction, 0.0)).xyz, Voxel(false, vec3(0)), false);
}

vec4 get_albedo(Ray ray) {
    if (ray.hit) {
        Voxel voxel = voxels.voxels[ray.hit_voxel];
        return vec4(voxel.color, 1.0);
    }
    return vec4(0.0);
}

void trace_ray(Ray ray) {
    float t_x = 1.0 / ray.direction.x;
    float t_y = 1.0 / ray.direction.y;
    float t_z = 1.0 / ray.direction.z;
    float t_delta_x = 1.0;
    float t_delta_y = 1.0;
    float t_delta_z = 1.0;
    int step_x = 1;
    int step_y = 1;
    int step_z = 1;
    int x = int(ray.pos.x);
    int y = int(ray.pos.y);
    int z = int(ray.pos.z);
    if (ray.direction.x < 0.0) {
        step_x = -1;
        t_delta_x = -t_x;
    }
    if (ray.direction.y < 0.0) {
        step_y = -1;
        t_delta_y = -t_y;
    }
    if (ray.direction.z < 0.0) {
        step_z = -1;
        t_delta_z = -t_z;
    }
    float t_max_x = t_x * (1.0 - fract(ray.pos.x));
    float t_max_y = t_y * (1.0 - fract(ray.pos.y));
    float t_max_z = t_z * (1.0 - fract(ray.pos.z));
    while (!ray.hit) {
        if (t_max_x < t_max_y) {
            if (t_max_x < t_max_z) {
                x += step_x;
                t_max_x += t_delta_x;
            } else {
                z += step_z;
                t_max_z += t_delta_z;
            }
        } else {
            if (t_max_y < t_max_z) {
                y += step_y;
                t_max_y += t_delta_y;
            } else {
                z += step_z;
                t_max_z += t_delta_z;
            }
        }
        if (x < 0 || x >= voxels.width_x || y < 0 || y >= voxels.width_y || z < 0 || z >= voxels.width_z) {
            break;
        }
        Voxel voxel = get_voxel(x, y, z);
        if (voxel.occupied) {
            ray.hit_voxel = voxel;
            ray.hit = true;
        }
    }
}
