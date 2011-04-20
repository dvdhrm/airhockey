#version 120

struct light {
    vec4 pos;
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    float const_att;
    float lin_att;
    float quadr_att;
    float enable;
};

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat3 normal_matrix;

uniform light light0;

uniform vec4 material_diffuse;
uniform vec4 material_specular;
uniform vec4 material_ambient;
uniform vec4 material_emissive;
uniform float material_shine;

attribute vec3 a_Vertex;
attribute vec3 a_Color;
attribute vec3 a_Normal;

varying vec4 color;
varying vec3 normal;
varying vec3 lightDir;
varying vec3 halfVector;
varying vec4 vertDiffuse;
varying vec4 vertSpecular;
varying vec4 vertAmbient;
varying vec4 vertEmissive;
varying float vertShine;
varying float dist;
varying float att_const;
varying float att_lin;
varying float att_quadr;
varying float enable;

varying vec4 lightPos;
varying vec4 pos;

void main(void) {
    enable = light0.enable;
    pos = modelview_matrix * vec4(a_Vertex, 1.0);
    lightPos = modelview_matrix * light0.pos;
    normal = normalize(normal_matrix * a_Normal);

    lightDir = lightPos.xyz - pos.xyz;
    vec3 E = -(pos.xyz);

    dist = length(lightDir);
    lightDir = normalize(lightDir);
    halfVector = normalize(lightPos.xyz + E);

    vertDiffuse = material_diffuse * light0.diffuse;
    vertSpecular = material_specular * light0.specular;
    vertAmbient = material_ambient * light0.ambient;
    vertEmissive = material_emissive;
    vertShine = material_shine;

    att_const = light0.const_att;
    att_lin = light0.lin_att;
    att_quadr = light0.quadr_att;

    color = vec4(a_Color, 1.0);
    gl_Position = projection_matrix * pos;
}

