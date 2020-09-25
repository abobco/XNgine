#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 res;
uniform vec2 ball;
uniform vec2 box;

const vec2 box_dims = vec2(50.0,5.0);

#define BALL_RADIUS 5.0
#define SDF_RES 0.005 // field density
#define MERGE_RADIUS 0.1

// rounds off edges at shape intersection
float round_merge(float shape1, float shape2, float radius)
{
	vec2 intersection_space = vec2(shape1 - radius, shape2 - radius);
	intersection_space = min(intersection_space, 0.0);

	float insideDistance = -length(intersection_space);
	float simpleUnion = min(shape1, shape2);
	float outsideDistance = max(simpleUnion, radius);

	return  insideDistance + outsideDistance;
}

float sdCircle(vec2 point, float radius, vec2 texCoord){
	float scaled_radius = SDF_RES*radius; // scale down the radius to fit the resolution

	vec2 conv_factor = res.xy*SDF_RES;
	vec2 position = (texCoord - point)*conv_factor;

	return length(position) - scaled_radius;
}

float sdBox( in vec2 p, in vec2 b, vec2 texCoord )
{
    vec2 scaled_box = b*SDF_RES;

    vec2 conv_factor = res.xy*SDF_RES;
    vec2 position = (texCoord - p)*conv_factor;
    vec2 d = abs(position)-scaled_box;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

void main()
{
    float d1 = sdCircle(ball/res, BALL_RADIUS, fragTexCoord);
    float d2 = sdBox(box/res, box_dims, fragTexCoord);

    float d = round_merge( d1, d2, MERGE_RADIUS );
    vec3 col = vec3(1.0) - sign(d)*vec3(0.1,0.4,0.7);
    col *= exp(-12.0*abs(d));
    col = mix( col, vec3(1.0), 1.0-smoothstep(0.0,0.01,abs(d)) );
    
    gl_FragColor = vec4(col, 0.5);
}