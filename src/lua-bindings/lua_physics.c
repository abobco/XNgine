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

Transform lua_getTransform( lua_State *L, int table_stack_idx ) {
    Vector3 t = lua_getVector3Field( L, "position", table_stack_idx);
    Quaternion q = lua_getVector4Field( L, "rotation", table_stack_idx);
    Vector3 s = lua_getVector3Field( L, "scale", table_stack_idx);

    return (Transform) {t, q, s};
}

// returns a displacement vector for the collision in world space 
int lua_collision_AABB_sphere( lua_State *L ) { 
    Transform box = lua_getTransform(L, 1);
    Vector3 cen = lua_getVector3(L, 2);
    float r = luaL_checknumber(L, 3);
    
    Vector3 disp = collision_AABB_sphere(box, cen, r);
    print_vec(disp);
    lua_newtable(L);
    lua_setFloatField(L, "x", disp.x);
    lua_setFloatField(L, "y", disp.y);
    lua_setFloatField(L, "z", disp.z);

    return 1;
} 
