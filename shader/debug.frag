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
 */

// light parameters
uniform light lights[light_num];

// vertex processing lighting results
varying vec3 position_l[light_num];	// position in light coordinates
varying vec3 normal_l[light_num];	// surface normal in light coordinates
varying vec3 camera_l[light_num];	// camera position in light coordinates

// miscellaneous parameters
varying vec4 color;		// color of vertex

/*
 * Super ellipse computation for lighting parameters
 * Super ellipses allow to shape the light for almost any situation.
 * Takes as argument the light number \lnum and the position of the destination
 * \pos and returns the attenuation factor depending on the light shape.
 *
 * Currently disabled because my intel GMA does not support enough ALU
 * instructions so we need to reduce the code size.
 */
float super_ellipse_shape(int lnum)
{
	return 1.0;
}

/*
 * Compute single light
 */
vec3 compute_light_single(int lnum)
{
	vec3 N = -normalize(normal_l[lnum]);
	vec3 L = normalize(position_l[lnum]);
	vec3 V = normalize(camera_l[lnum] - position_l[lnum]);
	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	float attenuation = 1.0;
	attenuation = super_ellipse_shape(lnum);
	float l0 = NdotL;
	float l1 = NdotL;
	float l2 = l1 * NdotH;


	vec3 ambient = lights[lnum].color * vec3(color) * l0;
	vec3 diffuse = lights[lnum].color * vec3(color) * l1;
	vec3 specular = lights[lnum].color * l2;

	return attenuation * (ambient + diffuse + specular);
}

/*
 * Compute lighting
 */
vec3 compute_lights(void)
{
	int i;
	vec3 final = vec3(color) * 0.1;

	for (i = 0; i < light_num; ++i) {
		if (lights[i].enabled) {
			final += compute_light_single(i);
		}
	}

	return final;
}

void main(void)
{
	gl_FragColor = vec4(compute_lights(), 1.0);
}

