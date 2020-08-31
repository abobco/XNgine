#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raylib.h"
#include "raymath.h"
#include "math.h"
#include <stdlib.h>
// #include "../snake.h"
#include "../lua-bindings/lua_util.h"

#define EPSILON 1e-4    // I should probably be getting this value from the compiler but lazy

#define print_vec(a) printf("%s = (%f, %f, %f)\n", #a, a.x, a.y, a.z );

float rand_float();

float lerp(float a, float b, float t);
float mag_sqr(Vector2 input);
float mag(Vector2 input);

Vector2 normalize(Vector2 input);

Vector2 add_vec2(Vector2 a, Vector2 b);

Vector2 sub_vec2(Vector2 a, Vector2 b);

Vector2 lerp_vec2(Vector2 a, Vector2 b, float t);

Vector2 transform_vec2( Vector2 p, Vector2 origin, float angle );

float dot(Vector2 a, Vector2 b); 

int convex_poly_collision( Vector2* a, int a_n, Vector2 *b, int b_n );

int convex_poly_circle_collision(Vector2 *a, int a_n, Vector2 b_cen, float radius);

float perlin2d(float x, float y, float freq, int depth);

Vector3 transform_point( Vector3 point, Transform t );

Vector3 inv_transform_point( Vector3 point, Transform t );

Vector3 collision_AABB_sphere( Transform box, Vector3 cen, float r ); // returns a displacement vector for the collision in world space

#ifdef __cplusplus
}
#endif