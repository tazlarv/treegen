#version 330 core

in vec3 set_color;
in float flogz;

out vec4 frag_color;

uniform float farPlaneCoef;

void main() {
	frag_color = vec4( set_color, 1.0f );
	gl_FragDepth = log2(flogz) * (farPlaneCoef * 0.5);
};
