#version 450

layout(location = 0) in vec3 in_tex_coords;
layout(location = 1) in vec3 in_frag_position;
layout(location = 2) in vec3 in_camera_position;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler3D volume;

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    vec3 ray_dir = normalize(in_frag_position - in_camera_position);
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

        vec4 density = texture(volume, ray_pos);
        vec4 sample_color = vec4(0.0, 0.0, 0.0, 0.0);
        if (density.x > 140.0) {
            sample_color = vec4(1.0, 1.0, 1.0, 0.25);
        }
        color = (1.0 - color.w) * sample_color + color;
        ray_pos += ray_dir * step_size;
    }

    out_color = color;
}