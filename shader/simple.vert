#version 120

uniform mat4 mpe_mat;		// modelview, projection and eye matrix

attribute vec4 position_in;

void main(void) {
	gl_Position = mpe_mat * position_in;
}

