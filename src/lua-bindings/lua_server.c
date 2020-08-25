#include "lua_util.h"
#include "../servers/message_queue.h"
#include "../servers/bt/gamepad_server.h"

static pthread_t pair_thread_id;

int lua_getConnections(lua_State *L) {
    int result = get_connections();

    lua_pushnumber(L, result);

    return 1;
}

int lua_get_connected(lua_State *L) {
    int id = luaL_checkinteger(L, 1);

    lua_pushboolean( L, get_connection_ids()[id] != NULL );

    return 1;
}

int lua_getMotionData(lua_State *L) {
    int player_index = luaL_checkinteger(L, 1);

    vec2 result = get_motion_data(player_index);
    
    lua_pushnumber(L, result.x);
    lua_pushnumber(L, result.y);

    // printf("gyro data: ( %f, %f) \n", result.x, result.y);
    return 2;
}

// server fns 
MessageList *lua_allocateMessageBuffer( lua_State *L, int n) {
    size_t nbytes = sizeof(MessageList) + (n-1)*sizeof(Message);
    MessageList *m = (MessageList*) lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, "Bob.MessageList");
    lua_setmetatable(L, -2);

    m->length = n;

    return m;
}

static MessageList *lua_checkMessageList( lua_State *L ) {
    void *ud = luaL_checkudata(L, 1, "Bob.MessageList");
    luaL_argcheck(L, ud != NULL, 1, "`message_list' expected");
    return (MessageList *)ud;
}

static Message *lua_getMessage( lua_State *L ) {
    MessageList *a = lua_checkMessageList(L);
    int idx = luaL_checkinteger(L, 2);

    luaL_argcheck(L, 1 <= idx && idx <= a->length, 2,
                "index out of range");

    // return element address
    return &a->buffer[idx - 1];
}

int lua_popNewMessages(lua_State *L) {
    int n = get_message_queue_size();
    //printf("%d\n", n);
    if ( n < 1 ) 
        return 0;   // no messages to pop
    MessageList *m = lua_allocateMessageBuffer(L, n); // allocate lua userdata
    MessageList poppedMessages = pop_new_messages();    // pop messages from the queue
    *m = poppedMessages;
    return 1;
}

int lua_getMessageValue (lua_State *L) {
    Message m = *lua_getMessage(L);

    lua_newtable(L);
    lua_setIntField(L, "id", m.user_id);
    lua_setIntField(L, "type", m.type);
    lua_setIntField(L, "user_type", m.user_type);
    lua_setFloatField(L, "x", m.motion.x);
    lua_setFloatField(L, "y", m.motion.y);

    return 1;
}

int lua_messageListSize (lua_State *L) {
    MessageList *a = (MessageList *) lua_checkMessageList(L);
    luaL_argcheck(L, a != NULL, 1, "`message_list' expected");
    lua_pushnumber(L, a->length);
    return 1;
}

int lua_messageListToString (lua_State *L) {
    MessageList *a = lua_checkMessageList(L);
    lua_pushfstring(L, "message_list(%d)", a->length);
    return 1;
}

int lua_setPairMode(lua_State *L) {
    int mode = 0;
    if ( lua_isboolean(L, 1) )
        mode = lua_toboolean(L, 1);

    if ( mode == true ) {
        pthread_create(&pair_thread_id, NULL, sync_loop, NULL);
    } else {
        set_sync_interrupt(true);
    }
      
    return 0;
}

int lua_openServerLib(lua_State *L) {
    luaL_newmetatable(L, "Bob.MessageList");
    
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // push the metatable
    lua_settable(L, -3); // metatable.__index = metatable
    luaL_setfuncs(L, lua_server_m, 0);

    lua_newtable(L);
    luaL_setfuncs(L, lua_server_f, 0);
    lua_setglobal(L, "server");

    return 1;
}