#version 120

/*
 * Shader constants
 * \light_num is the number of lights that are supported. A light is set with a
 * "struct light" datatype. If \light->enabled is false, then the light is
 * skipped in computation.
 */

const int light_num = 2;

struct light {
	bool enabled;			// light enabled
	vec3 color;			// light color

	mat4 mat;			// world to light coordinate matrix
	mat4 mat_it;			// same but inverse transpose
};

/*
 * Uniforms
 * All coordinates are by default in world space. All matrices convert from/to
 * world space if not otherwise given.
 */

// light parameters
uniform light lights[light_num];	// light parameters

// modelview, projection and eye matrices
uniform mat4 m_mat;			// modelview matrix
uniform mat4 m_mat_it;			// modelview matrix inverse transpose
uniform mat4 mpe_mat;			// modelview, projection and eye matrix

// miscellaneous incoming parameters
uniform vec4 cam_pos;			// position of camera

// position, color and normal of incoming vertex
attribute vec4 position_in;
attribute vec4 color_in;
attribute vec4 normal_in;

// outgoing light paramaters
varying vec3 position_l[light_num];	// position in light coordinates
varying vec3 normal_l[light_num];	// surface normal in light coordinates
varying vec3 camera_l[light_num];	// camera position in light coordinates

// miscellaneous outgoing parameters
varying vec4 color;			// color of vertex

/*
 * Read incoming light arguments and compute the light parameters for the
 * fragment processing.
 */
void compute_lights(void)
{
	vec4 w_pos;
	vec4 w_nor;
	int i;

	w_pos = m_mat * position_in;
	w_nor = m_mat_it * normal_in;

	for (i = 0; i < light_num; ++i) {
		if (lights[i].enabled) {
			position_l[i] = (lights[i].mat * w_pos).xyz;
			normal_l[i] = (lights[i].mat_it * w_nor).xyz;
			camera_l[i] = (lights[i].mat * cam_pos).xyz;
		}
	}
}

void main(void)
{
	compute_lights();

	color = color_in;
	gl_Position = mpe_mat * position_in;
}
