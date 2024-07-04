#version 450 core

layout(location = 0) in vec2 in_pos;  // pos on the screen (-1, 1).

layout(location = 0) out vec4 out_color;  // color of the pixel.

layout(std140, binding = 0) uniform Camera {
    vec3 pos;
    vec3 dir;
    vec3 up;
}
camera;

struct Voxel {
    bool occupied;
    vec3 color;
};

struct Ray {
    vec3 pos;
    vec3 direction;
    int depth;
    Voxel hit_voxel;
    bool hit;
};

struct DDAState {
    ivec3 step;
    uint X, Y, Z;
    vec3 tMax, tDelta;
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
    Ray ray;
    ray.pos = camera.pos;
    ray.direction = normalize(camera.dir + pos.y * camera.up + pos.x * cross(camera.dir, camera.up));
    ray.depth = 0;
    ray.hit = false;
    return ray;
}

vec4 get_albedo(Ray ray) {
    if (ray.hit) {
        Voxel voxel = ray.hit_voxel;
        return vec4(voxel.color, 1.0);
    }
    return vec4(0);
}

bool point_in_bounds(vec3 point) {
    return point.x >= 0 && point.x < voxels.width_x &&
           point.y >= 0 && point.y < voxels.width_y &&
           point.z >= 0 && point.z < voxels.width_z;
}

void trace_ray(in Ray ray, out Ray ray_out) {

}

void main() {
    Ray ray = get_ray(in_pos);
    trace_ray(ray, ray);
    out_color = get_albedo(ray);
}
