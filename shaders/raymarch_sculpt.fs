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

float sdSphere( in vec3 pos, float rad )
{
    return length(pos)-rad;
}

float sdElipsoid( in vec3 pos, in vec3 rad )
{
    float k0 = length(pos/rad);
    float k1 = length(pos/rad/rad);
    return k0*(k0-1.0)/k1;
}

float smin( in float a, in float b, float k )
{
    float h = max( k - abs(a-b), 0.0);
    return min(a,b) - h*h/(k*4.0);
}

float smax( in float a, in float b, float k ) 
{
    float h = max( k - abs(a-b), 0.0);
    return max(a,b) + h*h/(k*4.0);
}

vec2 sdGuy( in vec3 pos )
{
    float t = 0.5; //fract(iTime);
    
    float y = 4.0*t*(1.0-t);
    float dy = 4.0*(1.0-2.0*t);
    
    vec2 u = normalize( vec2( 1.0, -dy ) );
    vec2 v = vec2( dy, 1.0 );
        
    vec3 cen = vec3(0.0,y,0.0);
    
    float sy = 0.5 + 0.5*y; 
    float sz = 1.0/sy;
    
 	vec3 rad = vec3(0.25, 0.25*sy, 0.25*sz);
    
    vec3 q = pos-cen;
    
     // q.yz = vec2( dot(u, q.yz), dot(v,q.yz) );
    
    float d = sdElipsoid(q, rad);
    
    // vec3 h = q;
    // vec3 sh = vec3( abs(h.x), h.yz);
    
    // // head
    // float head_d = sdElipsoid(h- vec3(0.0, 0.28, 0.0), vec3(0.15, 0.2, 0.23));
    // float noggin_d = sdElipsoid(h- vec3(0.0, 0.28, -0.1), vec3(0.23, 0.2, 0.2));
    
    // head_d = smin(head_d,noggin_d,0.05);
    // d = smin(d, head_d, 0.15);
    
    // eyebrows
    // vec3 eb = sh-vec3(0.12, 0.34, 0.15);
    // eb.xy = (mat2(3,4,-4,3)/5.0)*eb.xy; 
        
    // head_d = sdElipsoid(eb, vec3(0.06, 0.035, 0.05));
    // d = smin(d, head_d, 0.04);
    
    // // mouth
    // head_d = sdElipsoid(h - vec3(0.0, 0.05, 0.15), vec3(0.1, 0.04, 0.3));
    // d = smax(d, -head_d, 0.03);
    
    vec2 res = vec2(d, 2.0);	// 2 == head material id
    
    // // eye
    // float eye_d = sdSphere( sh - vec3(0.08, 0.28, 0.16), 0.05);
    // if ( eye_d < d ) res = vec2(eye_d, 3.0);	// 3 == eye material id
    
    // eye_d = sdSphere( sh - vec3(0.09, 0.28, 0.18), 0.02);
    // if ( eye_d < d ) res = vec2(eye_d, 4.0);	// 4 == cornea material id
    
    
    return res;
}

// sphere sdf
vec2 map( in vec3 pos ) 
{
	vec2 d1 = sdGuy(pos);
    
    
    float d2 = pos.y - (-0.25);
    
    return (d2<d1.x) ? vec2(d2, 1.0) : d1;
}

vec3 calcNormal( in vec3 pos )
{
    vec2 e = vec2(0.0001, 0.0); // gradient value
        
    // estimate surface normal by checking distance to near points
    return normalize( vec3(map(pos+e.xyy).x - map(pos-e.xyy).x,
                           map(pos+e.yxy).x - map(pos-e.yxy).x,
                           map(pos+e.yyx).x - map(pos-e.yyx).x ));
}

float castShadow( in vec3 rayOrigin, vec3 rayDir ) 
{
    float res = 1.0;
    
    float t = 0.001;
    for ( int i =0; i< 100; i++ ) 
    {
    	vec3 pos = rayOrigin +t*rayDir;
        float h = map(pos).x;
        res = min( res, 16.0*h/t );
        if ( h < 0.0001 ) break;
        
        t += h;
        if ( t>20.0 ) break;
    }
    
    return clamp(res, 0.0, 1.0);
}

vec2 castRay( in vec3 rayOrigin, in vec3 rayDir ) 
{
    float m = -1.0;
    float t = 0.0;
    for ( int i=0; i <100; i++ ) {
   		vec3 pos = rayOrigin + t*rayDir;
        
        vec2 h = map( pos );
        m = h.y;
        if ( h.x<0.001 ) break; // point is inside of the sphere
        t+= h.x;
        if ( t>20.0 ) break;  // ray didn't hit anything
    }
    if ( t > 20.0 ) m = -1.0;
    return vec2(t, m);
}

void main()
{
    // vec4 texelColor = texture2D(texture0, fragTexCoord);
    vec2 fragCoord = fragTexCoord*iResolution;
    vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
    
    float an = 0.1*iTime; // 10.0*iMouse.x/iResolution.x; //

    // camera
    vec3 target = vec3(0.0, 0.95, 0.0);
    vec3 rayOrigin = target + vec3(1.5*sin(an), 0.0, 1.5*cos(an));
    
    // camera axes
    vec3 ww = normalize(target - rayOrigin);
    vec3 uu = normalize( cross(ww, vec3(0,1,0)));
    vec3 vv = normalize( cross(uu, ww) );
    
    vec3 rayDir = normalize( p.x*uu + p.y*vv + 1.5*ww ); 
    
    vec3 col = vec3(0.4, 0.75, 1.0) - 0.7*rayDir.y;
    col = mix(col, vec3(0.7, 0.75, 0.8), exp(-10.0*rayDir.y) );
    
    vec2 tm = castRay( rayOrigin, rayDir );
    
    if ( tm.y > 0.0 ) { // ray hit something
        float t = tm.x;
		vec3 pos = rayOrigin + t*rayDir;
        vec3 norm = calcNormal(pos);
        
        vec3 mat = vec3(0.18);	// default gray material
        if ( tm.y < 1.5 ) {
            mat = vec3(0.05, 0.1, 0.02);
        } else if ( tm.y < 2.5 ) {
			mat = vec3(0.2, 0.1, 0.02 );
        } //else if ( tm.y < 3.5 ) {
		// 	mat = vec3(0.4 , 0.4, 0.4 );
        // } else if ( tm.y < 4.5 ) {
		// 	mat = vec3(0.02);
        // }         
        
        vec3 sun_dir = normalize( vec3(0.8, 0.4, 0.2) );
        float sun_diffuse = clamp( dot( norm, sun_dir ), 0.0, 1.0);
        float sun_shadow = castShadow( pos + norm*0.001, sun_dir);
        float sky_diffuse = clamp(0.5 +  0.5*dot(norm, vec3(0.0,1.0,0.0)), 0.0, 1.0);
        float bounce_dif = clamp(0.5 +  0.5*dot(norm, vec3(0.0,-1.0,0.0)), 0.0, 1.0);
        
        col  = mat*vec3( 8.0, 4.5, 3.0 )*sun_diffuse;// *sun_shadow;
        col += mat*vec3(0.5, 0.8, 0.9 )*sky_diffuse; 
        
        col += mat*vec3(0.7, 0.3, 0.2 )*bounce_dif; 
    }
    
    col = pow( col, vec3(0.4545) );	// gamma correction
    
    gl_FragColor = vec4(col, 1.0);
}