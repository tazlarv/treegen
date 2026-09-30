#version 330 core
layout (location = 0) in vec3 l_pos;
layout (location = 1) in vec2 l_tex_coords;
layout (location = 2) in vec3 l_normal;

out vs_output {
	vec3 world_pos;
	vec2 tex_coords;
	vec3 normal;
} vs_out;

out float flogz;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float farPlaneCoef;

uniform vec3 light_pos;
uniform vec3 view_pos;

void main() {
	vs_out.world_pos = vec3(model * vec4(l_pos, 1.0));
	vs_out.tex_coords = l_tex_coords;

	vs_out.normal = l_normal;

	gl_Position = projection * view * vec4( vs_out.world_pos, 1.0 );

	// Logarithmic depth buffer
	gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * (farPlaneCoef - 1.0);
	flogz = 1.0 + gl_Position.w;
};
