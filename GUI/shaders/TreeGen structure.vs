#version 330 core
layout ( location = 0 ) in vec3 l_pos;

out vec3 set_color;
out float flogz;

uniform vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float farPlaneCoef;

void main() {
	set_color = color;

	gl_Position = projection * view * model * vec4(l_pos, 1.0);

	// Logarithmic depth buffer
	gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * (farPlaneCoef - 1.0);
	flogz = 1.0 + gl_Position.w;
};