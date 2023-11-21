#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_tex_coords;

layout(location = 0) out vec3 out_tex_coords;
layout(location = 1) out vec3 out_frag_position;

layout(binding = 0) uniform Transformations {
	mat4 view;
	mat4 proj;
} u_transformations;

void main() {
	out_tex_coords = in_tex_coords;
	out_frag_position = in_position;

	gl_Position =
		u_transformations.proj *
		u_transformations.view *
		vec4(in_position, 1.0);
}