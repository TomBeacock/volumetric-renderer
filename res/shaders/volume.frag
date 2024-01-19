#version 450

layout(location = 0) in vec3 in_tex_coords;
layout(location = 1) in vec3 in_frag_position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 camera_position;
    float min_density;
    float max_density;
} u_ubo;
layout(binding = 1) uniform sampler3D u_volume;
layout(binding = 2) uniform sampler1D u_transfer_func;


void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 ray_dir = normalize(in_frag_position - u_ubo.camera_position);
    vec3 ray_pos = in_tex_coords;

    vec3 min_bounds = vec3(0.0, 0.0, 0.0);
    vec3 max_bounds = vec3(1.0, 1.0, 1.0);

    float ray_dist = 1.8;
    float step_size = 0.005;
    int steps = int(ray_dist / step_size);

    for (int i = 0; i < steps; i++) {
        if (any(greaterThan(ray_pos, max_bounds)) ||
            any(lessThan(ray_pos, min_bounds))) {
            break;
        }

        float density = texture(u_volume, ray_pos).r;
        float t = (density - u_ubo.min_density) / (u_ubo.max_density - u_ubo.min_density);
        vec4 sample_color = texture(u_transfer_func, t);
        color.rgb += color.a * (sample_color.a * sample_color.rgb);
        color.a *= (1.0 - sample_color.a);
        ray_pos += ray_dir * step_size;
    }

    color.a = 1.0 - color.a;
    out_color = color;
}