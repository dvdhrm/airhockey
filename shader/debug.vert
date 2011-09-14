#version 120

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 normal_matrix;

attribute vec4 a_Vertex;
attribute vec4 a_Color;
attribute vec4 a_Normal;

varying vec4 color;
varying vec4 pos;

void main(void) {
	pos = modelview_matrix * a_Vertex;

	color = a_Color;
	gl_Position = projection_matrix * pos;
}

