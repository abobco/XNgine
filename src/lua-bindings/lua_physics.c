#include "lua_util.h"
#include "../util/bob_math.h"

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

int lua_getConvexMeshBounds( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    MeshSet *p = &get_gamestate()->modelSet.convexMeshBounds[id];
    Model* model =  &get_gamestate()->modelSet.models[id];
    if ( p == NULL ) {
        printf("[PHYSICS] halfspace bounds not found! Not a convex mesh\n");
        return 0;
    }

    lua_newtable(L);
    for ( int i = 0; i < p->count; i++ ) {
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

int lua_MatrixTranslate( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    Vector3 t = lua_getVector3(L, 2);
    get_gamestate()->modelSet.models[id].transform = MatrixTranslate( t.x, t.y, t.z);
    return 0;
}