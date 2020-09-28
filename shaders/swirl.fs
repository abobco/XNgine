#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 resolution;

float radius = 80.0;
float angle = 0.8;

uniform vec2 center;

void main()
{
    vec2 tc = fragTexCoord*resolution;
    tc -= center;
    
    float dist = length(tc);

    if (dist < radius) 
    {
        float percent = (radius - dist)/radius;
        float theta = percent*percent*angle*8.0;
        float s = sin(theta);
        float c = cos(theta);
        
        tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));
    }

    tc += center;
    vec4 color = texture2D(texture0, tc/resolution)*colDiffuse*fragColor;;

    gl_FragColor = vec4(color.rgb, 1.0);;
}