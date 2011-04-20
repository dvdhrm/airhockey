#version 120

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

attribute vec3 a_Vertex;
attribute vec3 a_Color;
attribute vec3 a_Normal;

varying vec4 color;
varying vec4 pos;

void main(void) {
    pos = modelview_matrix * vec4(a_Vertex, 1.0);

    color = vec4(a_Color, 1.0);
    gl_Position = projection_matrix * pos;
}

