#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raylib.h"
#include "raymath.h"
#include "math.h"
#include <stdlib.h>
#include <float.h>

#define EPSILON 1e-6f    // I should probably be getting this value from the compiler but lazy

// requires C11 compiler
#define printf_dec_format(x) _Generic((x), \
    char: "%c", \
    signed char: "%hhd", \
    unsigned char: "%hhu", \
    signed short: "%hd", \
    unsigned short: "%hu", \
    signed int: "%d", \
    unsigned int: "%u", \
    long int: "%ld", \
    unsigned long int: "%lu", \
    long long int: "%lld", \
    unsigned long long int: "%llu", \
    float: "%f", \
    double: "%f", \
    long double: "%Lf", \
    char *: "%s", \
    void *: "%p")

#define bprint(x) printf("%s = ", #x), printf(printf_dec_format(x), x)  
#define bprintln(x) printf("%s = ", #x), printf(printf_dec_format(x), x), printf("\n");

#define fprint(f, x) fprintf(f, "%s = ", #x), fprintf(f, printf_dec_format(x), x)  
#define fprintln(f, x) fprintf(f, "%s = ", #x), fprintf(f, printf_dec_format(x), x), fprintf(f, "\n");

#define print_vec(a) printf("%s = (%f, %f, %f)\n", #a, a.x, a.y, a.z );

typedef struct Plane {
    Vector3 point;
    Vector3 normal;
} Plane;

typedef struct PlaneSet {
    Plane *planes;
    int count;
} PlaneSet;

int convex_poly_collision( Vector2* a, int a_n, Vector2 *b, int b_n );

int convex_poly_circle_collision(Vector2 *a, int a_n, Vector2 b_cen, float radius);

float perlin2d(float x, float y, float freq, int depth);

inline float rand_float() {
    return rand() / (float)(RAND_MAX);
}

inline float lerp(float a, float b, float t) {
    return a*t + b*(1.0f - t);
}

inline Vector3 transform_point( Vector3 point, Transform t ) {
    Vector3 scratch = Vector3Multiply(point, t.scale);
    scratch = Vector3RotateByQuaternion(scratch, t.rotation);
    return Vector3Add(point, scratch);
}

inline Vector3 inv_transform_point( Vector3 point, Transform t ) {
    Vector3 scratch = Vector3Subtract(point, t.translation);
    scratch = Vector3RotateByQuaternion(scratch, QuaternionInvert(t.rotation));
    return Vector3Multiply( scratch, (Vector3) { 1/t.scale.x, 1/t.scale.y, 1/t.scale.z } );
}

inline float halfspace_point( Vector3 plane, Vector3 normal, Vector3 point ) {
    Vector3 plane_to_point = Vector3Subtract(point, plane);
    return Vector3DotProduct(normal, plane_to_point);
}

inline bool point_in_AABB( Vector3 p, BoundingBox b ) {
    if ( p.x > b.min.x && p.y > b.min.y && p.z > b.min.z 
      && p.x < b.max.x && p.y < b.max.y && p.z < b.max.z )
      return true;
    return false;
}

inline Vector2 transform_vec2( Vector2 p, Vector2 origin, float angle ) {
    float c = cosf(angle);
    float s = sinf(angle);
    
    float tx = c*p.x - s*p.y;
    float ty = s*p.x + c*p.y;
    return (Vector2) { origin.x + tx, origin.y + ty};
}

// sphere-convex polyhedron separating axis test
inline int sep_axis_sphere(PlaneSet *bounds, Vector3 *sphere_cen, float sphere_rad,  Plane *closest, float *dist) {
    float closest_d = -FLT_MAX;

    for ( int i = 0; i < bounds->count; i++ ){
        Plane *plane = &bounds->planes[i];
        Vector3 closest_on_sphere = Vector3Scale(plane->normal, sphere_rad);
        closest_on_sphere = Vector3Subtract(*sphere_cen, closest_on_sphere);
        Vector3 separation = Vector3Subtract(closest_on_sphere, plane->point);
        float d = Vector3DotProduct(plane->normal, separation);
        
        if ( d > 0 ) {
            return 0;
        }
            
        if ( d > closest_d ) {
            closest_d = d;
            closest->normal = plane->normal;
            closest->point = plane->point;
        }
    }
    *dist = closest_d;
    return 1;
}

#ifdef __cplusplus
}
#endif