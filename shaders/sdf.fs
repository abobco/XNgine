#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 res; // = vec2(720, 480);

float radius = 100.0;
float angle = 0.8;

uniform vec2 cursor;
uniform float time;

uniform vec2 dot1;
uniform vec2 dot2;
uniform vec2 dot3;
uniform vec2 dot4;

float dot_radius = 25.0;

#define MERGE_RADIUS 0.05

// rounds off edges between edges of shapes
float round_merge(float shape1, float shape2, float radius)
{
	vec2 intersection_space = vec2(shape1 - radius, shape2 - radius);
	intersection_space = min(intersection_space, 0.0);

	float insideDistance = -length(intersection_space);
	float simpleUnion = min(shape1, shape2);
	float outsideDistance = max(simpleUnion, radius);

	return  insideDistance + outsideDistance;
}

float circleSDF(vec2 point, float radius, vec2 texCoord){
	float line_resolition = 0.005;	// controls density of the field lines
	float scaled_radius = line_resolition*radius;// scale down the radius to fit the resolution

	vec2 conv_factor = res.xy*line_resolition;
	vec2 position = (texCoord - point)*conv_factor;

	return length(position) - scaled_radius;

}

void main()
{
  float n1 = 0.1;
  float n2 = 0.1;

  // swirl effect
  vec2 tc = fragTexCoord*res;
  tc -= cursor;
    
  float dist = length(tc);

  if (dist < radius) 
  {
      float percent = (radius - dist)/radius;
      float theta = percent*percent*angle*8.0;
      float s = sin(theta);
      float c = cos(theta);
      
      tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));
  }
  tc += cursor;
  tc /= res;

  // float n1 = snoise(vec2(time, 0))*0.1;
  // float n2 = snoise(vec2(0, time))*0.1;



  // signed distance fields
  // vec2 p1 = vec2(0.58 + n2, 0.5 + n1);
  // vec2 p2 = vec2(0.43 + n1, 0.5 + n2);


  
	float d1 = circleSDF(dot1/res, dot_radius, tc);
	float d2 = circleSDF(dot2/res, dot_radius, tc);
	float d3 = circleSDF(dot3/res, dot_radius, tc);
	float d4 = circleSDF(dot4/res, dot_radius, tc);

  float d = round_merge( d1, d2, MERGE_RADIUS );
  d = round_merge(d,d3, MERGE_RADIUS);
  d = round_merge(d,d4, MERGE_RADIUS);
    
	// coloring
  vec3 col = vec3(1.0) - sign(d)*vec3(0.1,0.4,0.7);
  col *= 1.0 - exp(-3.0*abs(d));
	col *= 0.8 + 0.2*cos(150.0*d);
	col = mix( col, vec3(1.0), 1.0-smoothstep(0.0,0.01,abs(d)) );
    
  gl_FragColor = vec4(col, 1.0);//*colDiffuse;
}