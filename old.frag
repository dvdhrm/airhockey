#version 120

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
    if(enable > 0.0) {
        vec3 lightDir2 = lightPos.xyz - pos.xyz;
        vec3 E = -(pos.xyz);
        vec3 halfVector2 = lightPos.xyz + E;

        vec3 N = normalize(normal);
        float NdotL = max(dot(N, normalize(lightDir2)), 0.0);

        vec4 brightness = vec4(0.0);
        float attenuation = 1.0;

        if(NdotL > 0.0) {
            float NdotHV = max(dot(N, normalize(halfVector2)), 0.0);

            brightness += vertSpecular * pow(NdotHV, vertShine);
            brightness += vertDiffuse * NdotL;
            attenuation = 1.0 / (att_const +
                                att_lin * dist +
                                att_quadr * dist * dist);
        }

        gl_FragColor = ((brightness * attenuation) + vertAmbient + vertEmissive) * color;
        /*gl_FragColor = vec4(normalize(lightDir), 1.0);*/
        /*gl_FragColor = vec4(1.0, NdotL, 0.0, 1.0);*/
    }
    else {
        gl_FragColor = color;
    }
}

