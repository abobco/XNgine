#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raylib.h"
#include "raymath.h"
#include "math.h"
#include <stdlib.h>
// #include "../snake.h"
// #include "../lua-bindings/lua_util.h"

#define EPSILON 1e-4    // I should probably be getting this value from the compiler but lazy

#define print_vec(a) printf("%s = (%f, %f, %f)\n", #a, a.x, a.y, a.z );

typedef struct Plane {
    Vector3 point;
    Vector3 normal;
} Plane;

int convex_poly_collision( Vector2* a, int a_n, Vector2 *b, int b_n );

int convex_poly_circle_collision(Vector2 *a, int a_n, Vector2 b_cen, float radius);

float perlin2d(float x, float y, float freq, int depth);

inline float rand_float() {
    return (float)rand() / (float)(RAND_MAX);
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
    Vector2 res = { origin.x + tx, origin.y + ty};
    return res;
}

#ifdef __cplusplus
}
#endif