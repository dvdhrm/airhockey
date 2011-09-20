#version 120

uniform mat4 mpe_mat;		// modelview, projection and eye matrix

// position, color and normal of incoming vertex
attribute vec4 position_in;

varying vec4 color;		// color of vertex

void main(void) {
	color = vec4(1.0, 0.0, 0.0, 1.0);

	gl_Position = mpe_mat * position_in;
}

