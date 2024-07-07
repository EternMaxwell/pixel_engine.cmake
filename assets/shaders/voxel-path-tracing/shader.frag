#version 450 core

layout(location = 0) in vec2 in_pos;  // pos on the screen (-1, 1).

layout(location = 0) out vec4 out_color;  // color of the pixel.

layout(std140, binding = 0) uniform Camera {
    vec4 pos;
    vec4 dir;
    vec4 up;
    vec4 proj;
}
camera;

struct Voxel {
    vec4 data;
};

struct Ray {
    vec3 origin;
    vec3 dir;
    vec3 rdir;
    float t;
    vec3 d_sign;
    Voxel voxel;
    int axis;
    bool inside;
    bool hit;
};

struct DDAState {
    ivec3 step;
    uint X, Y, Z;
    float t;
    vec3 tMax, tDelta;
};

layout(std140, binding = 0) buffer Voxels {
    ivec4 size;
    Voxel voxels[];
}
voxels;

Voxel get_voxel(int x, int y, int z) {
    return voxels
        .voxels[x + y * voxels.size.x + z * voxels.size.x * voxels.size.y];
}

Ray create_ray(vec3 origin, vec3 dir) {
    Ray ray;
    ray.origin = origin;
    ray.dir = normalize(dir);
    ray.hit = false;
    ray.rdir = 1.0 / ray.dir;
    ray.d_sign = clamp(-sign(ray.dir), 0.0, 1.0);
    ray.axis = 0;
    return ray;
}

Ray get_ray() {
    vec3 origin = camera.pos.xyz;
    vec3 dir = normalize(
        camera.dir.xyz +
        tan(camera.proj.x) *
            (in_pos.y * camera.up.xyz -
             camera.proj.y * in_pos.x * cross(camera.dir.xyz, camera.up.xyz)));
    return create_ray(origin, dir);
}

vec4 get_albedo(Ray ray) {
    if (ray.hit) {
        Voxel voxel = ray.voxel;
        return vec4(voxel.data.zyw, 1.0);
    }
    return vec4(0);
}

float intersect_cube(inout Ray ray) {
    float tx1 = -ray.origin.x * ray.rdir.x;
    float tx2 = (voxels.size.x - ray.origin.x) * ray.rdir.x;
    float ty, tz, tmin = min(tx1, tx2), tmax = max(tx1, tx2);
    float ty1 = -ray.origin.y * ray.rdir.y;
    float ty2 = (voxels.size.y - ray.origin.y) * ray.rdir.y;
    ty = min(ty1, ty2);
    tmin = max(tmin, ty);
    tmax = min(tmax, max(ty1, ty2));
    float tz1 = -ray.origin.z * ray.rdir.z;
    float tz2 = (voxels.size.z - ray.origin.z) * ray.rdir.z;
    tz = min(tz1, tz2);
    tmin = max(tmin, tz);
    tmax = min(tmax, max(tz1, tz2));
    if (tmin == tz)
        ray.axis = 2;
    else if (tmin == ty)
        ray.axis = 1;
    return tmax >= tmin ? tmin : 1.0e34;
}

bool point_in_bounds(vec3 point) {
    return point.x >= 0 && point.x <= voxels.size.x && point.y >= 0 &&
           point.y <= voxels.size.y && point.z >= 0 && point.z <= voxels.size.z;
}

bool setup_dda(inout Ray ray, inout DDAState state) {
    state.t = 0;
    bool start_in_grid = point_in_bounds(ray.origin);
    if (!start_in_grid) {
        state.t = intersect_cube(ray);
        if (state.t > 1.0e33) return false;
    }
    state.step = ivec3(1 - ray.d_sign * 2);
    vec3 pos_in_grid = ray.origin + ray.dir * (state.t + 0.0001);
    vec3 grid_planes = ceil(pos_in_grid) - ray.d_sign;
    ivec3 P = clamp(ivec3(floor(pos_in_grid)), ivec3(0), voxels.size.xyz - 1);
    state.X = uint(P.x);
    state.Y = uint(P.y);
    state.Z = uint(P.z);
    state.tDelta = vec3(state.step) * ray.rdir;
    state.tMax = ray.rdir * (grid_planes - ray.origin);
    Voxel cell = get_voxel(P.x, P.y, P.z);
    ray.inside = start_in_grid && (cell.data.x > 0.5);
    return true;
}

void find_nearest(inout Ray ray) {
    int width_x = voxels.size.x;
    int width_y = voxels.size.y;
    int width_z = voxels.size.z;
    ray.origin += ray.dir * 0.001;
    DDAState state;
    if (!setup_dda(ray, state)) return;
    Voxel cell, lastCell;
    int axis = ray.axis;
    bool hit = false;
    if (ray.inside) {
        hit = true;
        while (true) {
            cell = get_voxel(int(state.X), int(state.Y), int(state.Z));
            if (cell.data.x < 0.5) {
                break;
            }
            lastCell = cell;
            if (state.tMax.x < state.tMax.y) {
                if (state.tMax.x < state.tMax.z) {
                    state.t = state.tMax.x;
                    state.X += uint(state.step.x);
                    axis = 0;
                    if (state.X >= width_x) break;
                    state.tMax.x += state.tDelta.x;
                } else {
                    state.t = state.tMax.z;
                    state.Z += uint(state.step.z);
                    axis = 2;
                    if (state.Z >= width_z) break;
                    state.tMax.z += state.tDelta.z;
                }
            } else {
                if (state.tMax.y < state.tMax.z) {
                    state.t = state.tMax.y;
                    state.Y += uint(state.step.y);
                    axis = 1;
                    if (state.Y >= width_y) break;
                    state.tMax.y += state.tDelta.y;
                } else {
                    state.t = state.tMax.z;
                    state.Z += uint(state.step.z);
                    axis = 2;
                    if (state.Z >= width_z) break;
                    state.tMax.z += state.tDelta.z;
                }
            }
        }
        if (state.t > 0.0) {
            ray.voxel = lastCell;
            ray.hit = hit;
        }
    } else {
        while (true) {
            cell = get_voxel(int(state.X), int(state.Y), int(state.Z));
            if (cell.data.x > 0.5) {
                hit = true;
                break;
            }
            if (state.tMax.x < state.tMax.y) {
                if (state.tMax.x < state.tMax.z) {
                    state.t = state.tMax.x;
                    state.X += uint(state.step.x);
                    axis = 0;
                    if (state.X >= width_x) break;
                    state.tMax.x += state.tDelta.x;
                } else {
                    state.t = state.tMax.z;
                    state.Z += uint(state.step.z);
                    axis = 2;
                    if (state.Z >= width_z) break;
                    state.tMax.z += state.tDelta.z;
                }
            } else {
                if (state.tMax.y < state.tMax.z) {
                    state.t = state.tMax.y;
                    state.Y += uint(state.step.y);
                    axis = 1;
                    if (state.Y >= width_y) break;
                    state.tMax.y += state.tDelta.y;
                } else {
                    state.t = state.tMax.z;
                    state.Z += uint(state.step.z);
                    axis = 2;
                    if (state.Z >= width_z) break;
                    state.tMax.z += state.tDelta.z;
                }
            }
        }
        if (state.t > 0) {
            ray.voxel = cell;
            ray.hit = hit;
        }
    }
    ray.t = state.t;
    ray.axis = axis;
}

vec4 trace_ray(Ray ray) {
    find_nearest(ray);
    if (ray.hit) {
        return get_albedo(ray);
    }
    vec3 sun_dir = normalize(vec3(1, 1, 1));
    float sun_intensity = max(0.0, pow(dot(sun_dir, ray.dir), 7));
    return vec4(
        0.7 * sun_intensity, 0.7 * sun_intensity, 0.7 * sun_intensity, 1.0);
}

void main() {
    Ray ray = get_ray();
    out_color = trace_ray(ray);
}
