#include "bob_math.h"
#include <stdio.h>
#include <float.h>

#include "../lua-bindings/lua_util.h"

#define NEAR_TAIL_LENGTH 1

struct Maxima {
    float min;
    float max;
};

struct Maxima projection_test(Vector2 *p, int n_sides, Vector2 axis) {
    struct Maxima m = { FLT_MAX, -FLT_MAX };

    for ( int i = 0; i < n_sides; i++ ) {
        float a1 = Vector2DotProduct(p[i], axis);
        
        // printf("a1: %f\n", a1);
        if ( a1 < m.min )
            m.min = a1;
        if ( a1 > m.max ) {
            m.max = a1;
        }
            
    }
    return m;
}

int sep_axis_test( Vector2 *a, int a_n, Vector2 *b, int b_n, Vector2 axis ) {
    int res = 1;
    struct Maxima am = projection_test(a, a_n, axis);
    struct Maxima bm = projection_test(b, b_n, axis);

    if ( bm.min > am.max || bm.max < am.min )
        res = 0;
        
   //  printf("col check # %d: axis=(%f, %f), maxima: a=(%f, %f) b=(%f, %f), res=%d\n",
        // i++, axis.x, axis.y, am.min, am.max, bm.min, bm.max, res);

    return res;
}

int convex_poly_collision( Vector2* a, int a_n, Vector2 *b, int b_n ) {
    int res = 1;
    // test against a's axes
    for ( int i = 0; i < a_n-1; i++) {
        Vector2 axis = {
            -(a[i+1].y - a[i].y),
            a[i+1].x - a[i].x
        };
        axis = Vector2Normalize(axis);
        res = sep_axis_test(a, a_n, b, b_n, axis);
        if ( res == 0 )
            return 0;
    }
    Vector2 axis_la = { 
      -(a[0].y - a[a_n-1].y),
        a[0].x - a[a_n-1].x
    };
    res = sep_axis_test(a, a_n, b, b_n, Vector2Normalize(axis_la));
    if ( res == 0 )
        return 0;

    // test against b's axes
    for ( int i = 0; i < b_n-1; i++) {
        Vector2 axis = {
            -(b[i+1].y - b[i].y),
            b[i+1].x - b[i].x
        };
        axis = Vector2Normalize(axis);
        res = sep_axis_test(b, b_n, a,a_n, axis);
        if ( res == 0 )
            return 0;
    }
    Vector2 axis_lb = { 
          b[0].y - b[b_n-1].y,
        -(b[0].x - b[b_n-1].x)
    };
    res = sep_axis_test(a, a_n, b, b_n, Vector2Normalize(axis_lb));
    if ( res == 0 )
        return 0;
    
    return res;
}

static struct Maxima circ_proj_test(Vector2 c, float r, Vector2 axis) {  
    float c_proj = Vector2DotProduct(c, axis);
    struct Maxima m = { c_proj - r, c_proj + r };
    return m;
}

static int circ_sep_axis_test( Vector2 *a, int a_n, Vector2 b_cen, float radius, Vector2 axis) {
    int res = 1;
    struct Maxima am = projection_test(a, a_n, axis);
    struct Maxima bm = circ_proj_test(b_cen, radius, axis);

    if ( bm.min > am.max || bm.max < am.min )
        res = 0;
        
   //  printf("col check # %d: axis=(%f, %f), maxima: a=(%f, %f) b=(%f, %f), res=%d\n",
        // i++, axis.x, axis.y, am.min, am.max, bm.min, bm.max, res);

    return res;
}

int convex_poly_circle_collision(Vector2 *a, int a_n, Vector2 b_cen, float radius) {
    int res = 1;

    // circle axis test
    Vector2 circ_axis;
    float closest_d = FLT_MAX;
    for ( int i = 0; i < a_n; i++ ) {
        Vector2 d = { a[i].x - b_cen.x, a[i].y - b_cen.y };
        
        float d_sqr = Vector2LengthSqr(d);
        if ( d_sqr < closest_d ) {
            closest_d = d_sqr;
            circ_axis = d;
        }
    }
    // res = circ_sep_axis_test(a, a_n, b_cen, radius, Vector2Normalize(circ_axis));
    res = circ_sep_axis_test(a, a_n, b_cen, radius, Vector2Normalize(circ_axis));
    if ( res == 0 )
        return 0;

    // polygon axis tests
    for ( int i = 0; i < a_n-1; i++) {
        Vector2 axis = {
            -(a[i+1].y - a[i].y),
            a[i+1].x - a[i].x
        };
        axis = Vector2Normalize(axis);
        res = circ_sep_axis_test(a, a_n, b_cen, radius, axis);
        if ( res == 0 )
            return 0;
    }
    Vector2 axis = { 
      -(a[0].y - a[a_n-1].y),
        a[0].x - a[a_n-1].x
    };
    // res = circ_sep_axis_test(a, a_n, b_cen, radius, axis);
    res = circ_sep_axis_test(a, a_n, b_cen, radius, Vector2Normalize(axis));
    if ( res == 0 )
        return 0;

    return res;
}

static void clamp_to_bound(float *m, Vector3 *b, Vector3 *d ) {
    float minval = *m;
    Vector3 bound = *b;
    Vector3 disp = *d;
    if ( fabs(bound.x) < minval )  {
        minval = fabs(bound.x);
        disp = (Vector3) { bound.x, 0, 0 };
    }
    if ( fabs(bound.y) < minval )  {
        minval = fabs(bound.y);
        disp = (Vector3) { 0, bound.y, 0 };
    }
    if ( fabs(bound.z) < minval )  {
        minval = fabs(bound.z);
        disp = (Vector3) { 0, 0, bound.z };
    }
    *m = minval; *d = disp;
}

// returns a displacement vector for the collision
Vector3 collision_AABB_sphere( Transform box, Vector3 cen, float r ) {
    box.scale = Vector3Scale(box.scale, 0.5);
    Vector3 local_sphere_cen = inv_transform_point( cen, box );

    BoundingBox bb = {
        (Vector3){-1,-1,-1},
        (Vector3){1,1,1},
    };

    Vector3 closest = {
        Clamp( local_sphere_cen.x, bb.min.x, bb.max.x ),
        Clamp( local_sphere_cen.y, bb.min.y, bb.max.y ),
        Clamp( local_sphere_cen.z, bb.min.z, bb.max.z )
    };

    if ( point_in_AABB(local_sphere_cen, bb) ) {
        // clamp sphere center to nearest box face
        Vector3 tomax = Vector3Subtract(bb.max, local_sphere_cen );
        Vector3 tomin = Vector3Subtract(bb.min, local_sphere_cen );
        float minval = FLT_MAX;
        Vector3 disp = { 0, 0, 0 };
        clamp_to_bound( &minval, &tomax, &disp ); 
        clamp_to_bound( &minval, &tomin, &disp ); 

        // add sphere radius
        Vector3 pen = Vector3Subtract(local_sphere_cen, disp);
        pen = Vector3Scale( Vector3Normalize(pen), (r+Vector3Length(pen)));
        return pen;
    }

    Vector3 offset = Vector3Subtract(local_sphere_cen, closest);
    float d = Vector3Length(offset);
    if ( d > r )
        return (Vector3) {0,0,0};
    
    print_vec(offset);
    Vector3 glob = transform_point( Vector3Scale( Vector3Normalize(offset), (r - d)), box );
    return glob;
}