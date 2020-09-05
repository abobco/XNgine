#include "lua_util.h"
#include "../util/bob_math.h"

Vector4 lua_getVector4( lua_State *L, int table_stack_idx) {
    Vector4 q;
    q.x = lua_checkFloatField(L, "x", table_stack_idx);
    q.y = lua_checkFloatField(L, "y", table_stack_idx);
    q.z = lua_checkFloatField(L, "z", table_stack_idx);
    q.z = lua_checkFloatField(L, "w", table_stack_idx);
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

Transform lua_getTransform( lua_State *L, int table_stack_idx ) {
    Vector3 t = lua_getVector3Field( L, "position", table_stack_idx);
    Quaternion q = lua_getVector4Field( L, "rotation", table_stack_idx);
    Vector3 s = lua_getVector3Field( L, "scale", table_stack_idx);

    return (Transform) {t, q, s};
}

int lua_getConvexMeshBounds( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    PlaneSet *p = &get_gamestate()->modelSet.convexMeshBounds[id];
    if ( p == NULL ) {
        printf("[PHYSICS] halfspace bounds not found! Not a convex mesh\n");
        return 0;
    }

    lua_newtable(L);
    for ( int i = 0; i < p->count; i++ ) {
        lua_pushinteger(L, i+1);
        lua_newtable(L);

        lua_pushstring(L, "point");
        lua_pushVector3(L, p->planes[i].point);
        lua_settable(L, -3);

        lua_pushstring(L, "normal");
        lua_pushVector3(L, p->planes[i].normal);
        lua_settable(L, -3);

        lua_settable(L, -3);
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

// returns a displacement vector for the collision in world space 
int lua_collision_AABB_sphere( lua_State *L ) { 
    Transform box = lua_getTransform(L, 1);
    Vector3 cen = lua_getVector3(L, 2);
    float r = luaL_checknumber(L, 3);
    
    Vector3 disp = collision_AABB_sphere(box, cen, r);
    print_vec(disp);
    lua_pushVector3(L, disp);

    return 1;
} 
