#include "raylib.h"

// Shader uniform data types
typedef enum {
    UNIFORM_FLOAT = 0,
    UNIFORM_VEC2,
    UNIFORM_VEC3,
    UNIFORM_VEC4,
    UNIFORM_INT,
    UNIFORM_IVEC2,
    UNIFORM_IVEC3,
    UNIFORM_IVEC4,
    UNIFORM_SAMPLER2D
} ShaderUniformDataType;

// Trace log type
typedef enum {
    LOG_ALL = 0,        
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
    LOG_NONE            
} TraceLogType;

 enum MSG_TYPE {
    MSG_DISCONNECT = -1,
    MSG_CONNECT,
    MSG_MOTION_VECTOR,
    MSG_STRING,
    MSG_QUIT,
    MSG_BTN_A,
    MSG_BTN_B,
    MSG_BTN_A_UP,
    MSG_BTN_B_UP
};

// Camera system modes
typedef enum {
    CAMERA_CUSTOM = 0,
    CAMERA_FREE,
    CAMERA_ORBITAL,
    CAMERA_FIRST_PERSON,
    CAMERA_THIRD_PERSON
} CameraMode;

// Camera projection modes
typedef enum {
    CAMERA_PERSPECTIVE = 0,
    CAMERA_ORTHOGRAPHIC
} CameraType;