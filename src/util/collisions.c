#include "bob_math.h"
#include <stdio.h>
#include <float.h>

#include "../lua-bindings/lua_util.h"

#define NEAR_TAIL_LENGTH 1

int sep_axis_sphere(PlaneSet *bounds, Vector3 *sphere_cen, float sphere_rad,  Plane *closest, float *dist);

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

static void closet_pt_point_OBB( Vector3 *p, OBB *b, Vector3 *q) {
    Vector3 d= Vector3Subtract(*p, b->cen);
    *q = b->cen;

    for ( int i = 0; i < 3; i++ ) {
        // project d onto the axis
        float dist = Vector3DotProduct(d, b->axes[i]);

        if ( dist >  b->extent[i] ) dist = b->extent[i];
        if ( dist < -b->extent[i] ) dist = -b->extent[i];
        
        *q = Vector3Add( *q,  Vector3Scale( b->axes[i], dist));
    } 
}

static float sq_dist_point_OBB( Vector3 *p, OBB *b ) {
    Vector3 closest;
    closet_pt_point_OBB(p, b, &closest);
    closest = Vector3Subtract(closest, *p);

    return Vector3DotProduct(closest, closest); 
}

void sphere_OBBs_collisions(Vector3 *sphere_cen, float sphere_rad, ModelSet *modelset ) {
    for ( int i = 0; i < modelset->count; i++ ) { 
        MeshSet *submeshes = &modelset->convexMeshBounds[i];
        for ( int j = 0; j < submeshes->box_count; j++ ) {
            OBB *box = &submeshes->boxes[j];
            Vector3 original_box_cen = box->cen;
            Vector3 original_axes[3];
            for (int k = 0; k < 3; k++ ) {
                original_axes[k] = box->axes[k];
            } 
            
            // transform to same coord system
            box->cen = Vector3Transform(box->cen, modelset->models[i].transform); 
            box->cen = Vector3Add(box->cen, modelset->positions[i]);
            for ( int k = 0; k < 3; k++ ) {
                box->axes[k] = Vector3Transform(box->axes[k], modelset->models[i].transform);
            }

            Vector3 closest;
            closet_pt_point_OBB(sphere_cen, box, &closest);
            Vector3 sphere_to_box = Vector3Subtract(closest, *sphere_cen);
            float dist = Vector3DotProduct(sphere_to_box, sphere_to_box); 

            if ( dist < EPSILON ) {
                // clamp sphere center to closest side
                // float closest_extent[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
                // float closest_d = FLT_MAX;
                // float extent_to_move = 0;
                // Vector3 axis_to_move = {0}; 
                // Vector3 d = Vector3Subtract(*sphere_cen, box->cen);
                
                // float d1 = Vector3Subtract(Vector3Scale(box->axes[0], box->extent[0]), d);
                // float d2 = Vector3Subtract(Vector3Scale(Vector3Negate(box->axes[0]), box->extent[0]), d);
                // if ( Vector3Subtract(Vector3Scale(box->axes[0], box->extent[0]), d) < closest_d ) {
                //     closest_d = d; 
                //     extent_to_move = box->extent[0];
                //     axis_to_move = box->axis[0];
                // } 
                // if ( Vector3Subtract(Vector3Scale(Vector3Negate(box->axes[0]), box->extent[0]), d) < closest_d ) {
                //     closest_d = d; 
                //     extent_to_move = Vector3Negate(box->axes[0]);
                //     axis_to_move = box->axis[0];
                // }
                    
            } else if ( dist < sphere_rad*sphere_rad ) {
                Vector3 n = Vector3Scale( Vector3Negate( Vector3Normalize(sphere_to_box) ), sphere_rad - dist);
                *sphere_cen = Vector3Add(*sphere_cen, n);
            }

            // transform back to local
            box->cen = original_box_cen;
            for (int k = 0; k < 3; k++ ) {
                box->axes[k] = original_axes[k];
            } 
        }
    }
}