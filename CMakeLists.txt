# Builds the project for raspberry pi in OpenGL ES 2.0 native mode (no x11 or GL driver)

cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
include(FetchContent)
# arm-hardfloat cross compiler toochain file
include(armhf-toolchain.cmake)
set(CMAKE_BUILD_TYPE Debug)
# raspi libraries for threading & gpu
include_directories( ${CMAKE_SYSROOT}/opt/vc/include ${CMAKE_SYSROOT}/opt/vc/include/interface/vmcs_host/linux ${CMAKE_SYSROOT}/opt/vc/include/interface/vcos/pthreads ${CMAKE_SYSROOT}/opt/vc/include/interface/vcos/generic)
link_directories(${CMAKE_SYSROOT}/opt/vc/lib)

# ================================== Build raylib =======================================

project(raylib)

# Check if raylib is already installed
find_package(raylib 3.0 QUIET) 

if (NOT raylib_FOUND) 
  # download raylib
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/master.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(PLATFORM "Raspberry Pi") # default platform is desktop
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the examples
    # build raylib
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

# ========================== Build libwebsockets ========================================

project(lws)

# LWS config
set(OPENSSL_ROOT_DIR ${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf/)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_TOOLCHAIN_FILE armhf-toolchain.cmake)
set(LWS_WITH_TLS ON)

 # build a debug symbol table for gdb
set(CMAKE_BUILD_TYPE Debug)

# fetch & build libwebsockets
FetchContent_Declare(
    libwebsockets
    GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
    GIT_TAG v4.0.10
    )
  FetchContent_GetProperties(libwebsockets)
  if(NOT libwebsockets_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(libwebsockets)
    add_subdirectory(${libwebsockets_SOURCE_DIR} ${libwebsockets_BINARY_DIR})
  endif()
  
  # libwebsockets build dependencies
  include_directories(build/_deps/libwebsockets-build/include ${CMAKE_SYSROOT}/usr/include/arm-linux-gnueabihf)
  link_directories(build/_deps/libwebsockets-build/lib ${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf/openssl-1.0.2)

# =======================================================================================

# ================================ Helper Library ========================================

project(XNlib)

set( xnlib  src/util/bob_math.h src/util/bob_math.c src/util/collisions.c src/util/file_io.c ${raylib_BINARY_DIR}/src/raylib.h )
list(APPEND xnlib src/servers/message_queue.c src/util/input.c src/servers/bt/gamepad_server.c) 
list(APPEND xnlib src/graphics/particles.c src/graphics/renderer.c)
set( lb src/lua-bindings )
# ${lb}/lua_snake.c 
list(APPEND xnlib ${lb}/lua_util.h ${lb}/lua_util.c ${lb}/lua_raylib.c ${lb}/lua_server.c ${lb}/lua_particles.c ${lb}/lua_physics.c )

link_directories(build/_deps/raylib-build/src)

add_library(xnlib-fn STATIC ${xnlib} )

# ========================================================================================

# =============================== Build the darn game ===================================

macro(XN_Build project_name src_file)
  project( ${project_name} ) 

  # build a debug symbol table for gdb
  set(CMAKE_BUILD_TYPE Debug)

  # build game
  add_executable(${PROJECT_NAME}  ${src_file} )

  # include_directories(build/_deps/libwebsockets-build/include ${raylib_BINARY_DIR}/src ${CMAKE_SYSROOT}/usr/include/lua5.1) # default raspi lua build
  include_directories(build/_deps/libwebsockets-build/include ${raylib_BINARY_DIR}/src ${raylib_BINARY_DIR}/src/external build/_deps/lua-5.4.0/install/include) # compound assignment lua5.4 build
  find_library( LUA54 NAMES "lua" PATHS ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/lua-5.4.0/install/lib NO_DEFAULT_PATH)

  # cross compiler stuff
  set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "./")
  target_link_libraries(${PROJECT_NAME} raylib vcos brcmGLESv2 brcmEGL pthread rt m bcm_host dl websockets ${LUA54} xnlib-fn )
  # target_link_libraries(${PROJECT_NAME} raylib vcos brcmGLESv2 brcmEGL pthread rt m bcm_host dl websockets ${LUA54} xnlib-fn profiler)
endmacro()

XN_Build( XNgine src/main.c )
XN_Build( XNgine++ src/main.cpp )
XN_Build( XNgl src/graphics/egl/egl_tests.c )
# set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -lglfw3 -lopengl32 -lgdi32 -Wall")
