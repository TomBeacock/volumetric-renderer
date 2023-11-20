#version 450

layout(location = 0) in vec3 in_tex_coords;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler3D volume;

void main() {
	vec3 offset_tex_coords = in_tex_coords - vec3(0.5, 0.0, 0.0);
	float val = texture(volume, offset_tex_coords).x;
	out_color = vec4(val, val, val, 1.0);
}