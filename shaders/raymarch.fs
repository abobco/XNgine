#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float iTime;
uniform vec2 iResolution;

// smooth union 
float smin( in float a, in float b, float k )
{
    float h = max( k - abs(a-b), 0.0);
    return min(a,b) - h*h/(k*4.0);
}

// smooth complement
float smax( in float a, in float b, float k ) 
{
    float h = max( k - abs(a-b), 0.0);
    return max(a,b) + h*h/(k*4.0);
}

float sdElipsoid( in vec3 pos,  vec3 rad )
{
    float k0 = length(pos/rad);
    float k1 = length(pos/rad/rad);
    return k0*(k0-1.0)/k1;
}

float sdSphere( in vec3 pos, float rad )
{
    return length(pos)-rad;
}

float map( in vec3 pos ) 
{

    // 2 spheres
    vec3 elip_cen = vec3(0.0, -0.2, 0.0);
	float d = sdElipsoid(pos + elip_cen, vec3(0.25, 0.5, 0.25));  // length(pos) - 0.25;
    float d2 = sdSphere(pos + elip_cen + vec3(-0.2, -0.2, 0.0), 0.25);
    
    // rounded merge
    d = smax(d, -d2, 0.04);
    
    // plane
    d2 = pos.y - (-0.25);
    
    return min(d,d2);
}

vec3 calcNormal( in vec3 pos )
{
    vec2 e = vec2(0.01, 0.0); // gradient value

    // estimate surface normal by checking distance to near points
    float x = map(pos+e.xyy) - map(pos-e.xyy);
    float y = map(pos+e.yxy) - map(pos-e.yxy);
    float z = map(pos+e.yyx) - map(pos-e.yyx);

    return normalize( vec3(x,y,z));

    // i think this doesn't work b/c no bit-shift support in opengl es 100
    // it would be faster tho
    // vec3 n = vec3(0.0);
    // for( int i=0; i<4; i++ )
    // {
    //     vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
    //     n += e*map(pos+0.0005*e,time).x;
    // }
    // return normalize(n);
}

float castRay( in vec3 rayOrigin, in vec3 rayDir ) 
{
    float t = 0.0;
    for ( int i=0; i <100; i++ ) {
   		vec3 pos = rayOrigin + t*rayDir;
        
        float h = map( pos );
        
        if ( h<0.001 ) break; // point is inside of the sphere
        t+= h;
        if ( t>20.0 ) break;  // ray didn't hit anything
    }
    if ( t > 20.0 ) t = -1.0;
    return t;
}

void main()
{
    // vec4 texelColor = texture2D(texture0, fragTexCoord);
    vec2 fragCoord = fragTexCoord*iResolution;
    vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;

    float an = iTime;

    // camera
    vec3 rayOrigin = vec3(1.5*sin(an), 0.0, 1.5*cos(an));
    vec3 target = vec3(0.0, 0.0, 0.0);
    
    // camera axes
    vec3 ww = normalize(target - rayOrigin);
    vec3 uu = normalize( cross(ww, vec3(0,1,0)));
    vec3 vv = normalize( cross(uu, ww) );
    
    vec3 rayDir = normalize( p.x*uu + p.y*vv + 1.5*ww ); 
    
    vec3 col = vec3(0.4, 0.75, 1.0) - 0.7*rayDir.y;
    col = mix(col, vec3(0.7, 0.75, 0.8), exp(-10.0*rayDir.y) );
    
    float t = castRay( rayOrigin, rayDir );
    
    if ( t > 0.0 ) { // ray hit something
		vec3 pos = rayOrigin + t*rayDir;
        vec3 norm = calcNormal(pos);
        
        vec3 mat = vec3(0.18);	// gray material color
        
        vec3 sun_dir = normalize( vec3(0.8, 0.4, 0.2) );
        float sun_diffuse = clamp( dot( norm, sun_dir ), 0.0, 1.0);
        float sun_shadow = step( castRay( pos + norm*0.001, sun_dir), 0.0);
        float sky_diffuse = clamp(0.5 +  0.5*dot(norm, vec3(0.0,1.0,0.0)), 0.0, 1.0);
        float bounce_dif = clamp(0.5 +  0.5*dot(norm, vec3(0.0,-1.0,0.0)), 0.0, 1.0);
        
        col  = mat*vec3( 8.0, 4.5, 3.0 )*sun_diffuse*sun_shadow;
        col += mat*vec3(0.5, 0.8, 0.9 )*sky_diffuse; 
        
        col += mat*vec3(0.7, 0.3, 0.2 )*bounce_dif; 
    }
    
    col = pow( col, vec3(0.4545) );	// gamma correction
    
    // gl_FragColor = texelColor*colDiffuse;
    gl_FragColor = vec4(col, 1.0);
}