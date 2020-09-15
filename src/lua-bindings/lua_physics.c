#include "lua_util.h"
#include "../util/bob_math.h"
#include "../graphics/renderer.h"
#include <float.h>

Vector4 lua_getVector4( lua_State *L, int table_stack_idx) {
    Vector4 q;
    q.x = lua_checkFloatField(L, "x", table_stack_idx);
    q.y = lua_checkFloatField(L, "y", table_stack_idx);
    q.z = lua_checkFloatField(L, "z", table_stack_idx);
    q.w = lua_checkFloatField(L, "w", table_stack_idx);
    return q;
}

Vector4 lua_getVector4Field( lua_State *L, const char *key, int table_stack_idx ) {
    lua_pushstring(L, key);
    lua_gettable(L, table_stack_idx);
    return lua_getVector4(L, -1);
}

void lua_pushVector3(lua_State *L, Vector3 v) {
    lua_newtable(L);
    lua_setFloatField(L, "x", v.x);
    lua_setFloatField(L, "y", v.y);
    lua_setFloatField(L, "z", v.z);
}

void lua_pushVector4( lua_State *L,  Vector4 v) {
    lua_newtable(L);
    lua_setFloatField(L, "x", v.x);
    lua_setFloatField(L, "y", v.y);
    lua_setFloatField(L, "z", v.z);
    lua_setFloatField(L, "w", v.w);
}

int lua_Slerp( lua_State *L ) {
    Quaternion q1 = lua_getVector4(L, 1);
    Quaternion q2 = lua_getVector4(L, 2);
    float t = luaL_checknumber(L, 3);

    lua_pushVector4(L, QuaternionSlerp(q1, q2, t));

    return 1;
} 

Transform lua_getTransform( lua_State *L, int table_stack_idx ) {
    Vector3 t = lua_getVector3Field( L, "position", table_stack_idx);
    Quaternion q = lua_getVector4Field( L, "rotation", table_stack_idx);
    Vector3 s = lua_getVector3Field( L, "scale", table_stack_idx);

    return (Transform) {t, q, s};
}

static void extreme_points_in_direction(Vector3 dir, float *verts, int n, int *imin, int *imax) {
    float min_proj = FLT_MAX;
    float max_proj = -FLT_MAX;
    for ( int i = 0; i < n; i++ ) {
        Vector3 vert = get_vert(verts, i*3);
        float proj = Vector3DotProduct(vert, dir);
        
        if ( proj < min_proj ) {
            min_proj = proj;
            *imin = i;
        }
        if ( proj > max_proj ) {
            max_proj = proj;
            *imax = i;
        }
    }
}

typedef struct AABB {
    Vector3 min;
    Vector3 max;
} AABB;

static AABB get_model_AABB( Model *model ) {
    AABB box = {
        (Vector3) {FLT_MAX, FLT_MAX, FLT_MAX},
        (Vector3) {-FLT_MAX, -FLT_MAX, -FLT_MAX},
    };

    Vector3 axes[3] = {
            (Vector3) {1, 0, 0},
            (Vector3) {0, 1, 0},
            (Vector3) {0, 0, 1}
    };

    for ( int i = 0; i < model->meshCount; i++ ) {
        for ( int j = 0; j < 3; j++ ) {
            int imin, imax;
            extreme_points_in_direction(
                axes[j], 
                model->meshes[i].vertices, 
                model->meshes[i].vertexCount,
                &imin, &imax
            );
            Vector3 min_vert = get_vert(model->meshes[i].vertices, imin*3);
            Vector3 max_vert = get_vert(model->meshes[i].vertices, imax*3);
            if ( min_vert.x < box.min.x ) box.min.x = min_vert.x;
            if ( min_vert.y < box.min.y ) box.min.y = min_vert.y;
            if ( min_vert.z < box.min.z ) box.min.z = min_vert.z;
            if ( max_vert.x > box.max.x ) box.max.x = max_vert.x;
            if ( max_vert.y > box.max.y ) box.max.y = max_vert.y;
            if ( max_vert.z > box.max.z ) box.max.z = max_vert.z;
        }
    }
    return box;
}

int lua_getBoundingSphere( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    Model* model =  &get_gamestate()->modelSet.models[id];

    AABB box =  get_model_AABB(model);
    
    float radius = 0;
    float dist_to_verts[] = {
        Vector3Length(box.min),
        Vector3Length(box.max)
    };
    if ( radius < dist_to_verts[0] ) radius = dist_to_verts[0];
    if ( radius < dist_to_verts[1] ) radius = dist_to_verts[1];

    get_gamestate()->modelSet.boundingSpheres[id] = radius;

    // printf("%f\n", radius);

    lua_pushnumber(L, radius);
    return 1;
}

int lua_getConvexMeshBounds( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    MeshSet *p = &get_gamestate()->modelSet.convexMeshBounds[id];
    Model* model =  &get_gamestate()->modelSet.models[id];
    if ( p == NULL ) {
        printf("[PHYSICS] halfspace bounds not found! Not a convex mesh\n");
        return 0;
    }

    lua_newtable(L);
    for ( int i = 0; i < p->mesh_count; i++ ) {
        lua_pushinteger(L, i+1);
        lua_newtable(L);
        for ( int j = 0; j < p->meshes[i].count; j++ ) {
            lua_pushinteger(L, j+1);
            lua_newtable(L);
            Plane* plane = &p->meshes[i].planes[j];
            Vector3 point = Vector3Transform(plane->point, model->transform);
            Vector3 normal = Vector3Transform(plane->normal, model->transform);

            lua_pushstring(L, "point");
            lua_pushVector3(L, point);
            lua_settable(L, -3);

            lua_pushstring(L, "normal");
            lua_pushVector3(L, normal);
            lua_settable(L, -3);

            lua_settable(L, -3);
        }
        lua_settable(L, -3);
    }

    return 1;
}

int lua_separatingAxisSphere(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    Vector3 sphere_cen = lua_getVector3(L, 2);
    float sphere_rad = luaL_checknumber(L, 3);

    MeshSet *meshset = &get_gamestate()->modelSet.convexMeshBounds[id];
    Model *model =  &get_gamestate()->modelSet.models[id];
    Vector3 *poly_pos = &get_gamestate()->modelSet.positions[id];
    float bounding_sphere_radius = get_gamestate()->modelSet.boundingSpheres[id];

    // transform sphere to local space of the polyhedron
    Vector3 local_sphere = Vector3Subtract(sphere_cen, *poly_pos);
    lua_newtable(L);
    if ( Vector3LengthSqr(local_sphere) > bounding_sphere_radius*bounding_sphere_radius )
        return 1;

    
    Matrix inverse_transform = MatrixInvert(model->transform);
    local_sphere = Vector3Transform(local_sphere, inverse_transform);

    int overlap_count = 0;
    for ( int i =0; i < meshset->mesh_count; i++ ) {
        PlaneSet *bounds = &meshset->meshes[i];

        Plane closest;
        float dist;
        if ( sep_axis_sphere( bounds, &local_sphere, sphere_rad, &closest, &dist ) ) {
            // transform collision data to world space
            closest.point = Vector3Transform(closest.point, model->transform);
            closest.point = Vector3Add(closest.point, *poly_pos);
            closest.normal = Vector3Transform(closest.normal, model->transform);

            lua_pushinteger(L, ++overlap_count);
            lua_newtable(L);

                lua_pushstring(L, "d");
                lua_pushnumber(L, dist);
                lua_settable(L, -3);

                lua_pushstring(L, "s");
                lua_newtable(L);

                    lua_pushstring(L, "point");
                    lua_pushVector3(L, closest.point);
                    lua_settable(L, -3);

                    lua_pushstring(L, "normal");
                    lua_pushVector3(L, closest.normal);
                    lua_settable(L, -3);

                lua_settable(L, -3);

            lua_settable(L, -3);
        }
    }
    
    return 1;
}
 
int lua_rotateVectorEulers( lua_State *L ) {
    Vector3 vec = lua_getVector3(L, 1);
    Vector3 eul = lua_getVector3(L, 2);
    
    Quaternion q = QuaternionFromEuler(eul.x, eul.y, eul.z);
    lua_pushVector3(L, Vector3RotateByQuaternion(vec, q));
    return 1;
}

int lua_rotateVectorByQuaternion( lua_State *L ) {
    Vector3 vec = lua_getVector3(L, 1);
    Quaternion q = lua_getVector4(L, 2);
    
    lua_pushVector3(L, Vector3RotateByQuaternion(vec, q));
    return 1;
}

int lua_QuaternionFromEuler( lua_State *L ) {
    Vector3 eul = lua_getVector3(L, 1);
    lua_pushVector4(L, QuaternionFromEuler(eul.x, eul.y, eul.z));
    return 1;
}

int lua_setModelTransform( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    Vector3 trans = lua_getVector3(L, 2);
    Vector3 rot = lua_getVector3(L, 3);
    Matrix translation = MatrixTranslate( trans.x, trans.y, trans.z);
    Matrix rotation = MatrixRotateXYZ( rot );

    get_gamestate()->modelSet.models[id].transform = MatrixMultiply(rotation, translation);

    return 0;
}

int lua_MatrixTranslate( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    Vector3 t = lua_getVector3(L, 2);
    get_gamestate()->modelSet.models[id].transform = MatrixTranslate( t.x, t.y, t.z);
    return 0;
}

int lua_setModelPosition( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    get_gamestate()->modelSet.positions[id] = lua_getVector3(L, 2);

    return 0;
}

int lua_transformVectorByMatrix( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    Vector3 vec = lua_getVector3(L, 2);
    lua_pushVector3(L, Vector3Transform(vec, get_gamestate()->modelSet.models[id].transform ) );

    return 1;
}