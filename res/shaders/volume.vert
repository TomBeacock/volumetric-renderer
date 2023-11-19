#version 450

layout(location = 0) in vec3 in_position;

layout(binding = 0) uniform Transformations {
	mat4 view;
	mat4 proj;
} u_transformations;

void main() {
	gl_Position =
		u_transformations.proj *
		u_transformations.view *
		vec4(in_position, 1.0);
}