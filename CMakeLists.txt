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

set( lb src/lua-bindings )

set( xnlib  
  src/util/file_io.c src/util/linked_list.c src/util/fbxloader/fbxloader.c
  src/util/bob_math.h src/util/bob_math.c src/util/collisions.c 
  ${raylib_BINARY_DIR}/src/raylib.h 
  src/servers/message_queue.c src/util/input.c src/servers/bt/gamepad_server.c
  src/graphics/particles.c src/graphics/renderer.c
  ${lb}/lua_util.h ${lb}/lua_util.c ${lb}/lua_raylib.c ${lb}/lua_server.c ${lb}/lua_particles.c ${lb}/lua_physics.c
)

link_directories(build/_deps/raylib-build/src)

add_library(xnlib-fn STATIC ${xnlib} )

# ========================================================================================

project(robolib_build)

set( arducam_dir src/SPI_Camera/src/ )

set( robolib_src  
    src/ws/message_queue.c 
    ${arducam_dir}EZCAM.c 
    ${arducam_dir}ArduCAM.c 
    ${arducam_dir}bcm283x_board_driver.c 
    ${arducam_dir}sccb_bus.c
)

# link_directories(build/_deps/raylib-build/src)

add_library(robolib STATIC ${robolib_src} )

# =============================== Build the darn game ====================================

option(Profiling "build with gperftools, record profiling stats")
set(Profiling FALSE)

macro(XN_Build project_name src_file)
  project( ${project_name} ) 

  # build a debug symbol table for gdb
  set(CMAKE_BUILD_TYPE Debug)

  if(Profiling)
    add_definitions( -DPROFILING )
    set(prof profiler)
  else()
    set(prof "")
  endif()

  # build game
  add_executable(${PROJECT_NAME}  ${src_file} )

  # include_directories(build/_deps/libwebsockets-build/include ${raylib_BINARY_DIR}/src ${CMAKE_SYSROOT}/usr/include/lua5.1) # default raspi lua build
  include_directories(build/_deps/libwebsockets-build/include ${raylib_BINARY_DIR}/src ${raylib_BINARY_DIR}/src/external build/_deps/lua-5.4.0/install/include) # compound assignment lua5.4 build
  find_library( LUA54 NAMES "lua" PATHS ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/lua-5.4.0/install/lib NO_DEFAULT_PATH)

  find_library(PIG NAMES libpigpio.so.1 PATHS src/pigpio-master/install/lib/ NO_DEFAULT_PATH )
  find_library(PIGD NAMES libpigpiod_if2.so.1 PATHS src/pigpio-master/install/lib/ NO_DEFAULT_PATH )


  # cross compiler stuff
  set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "./")
  target_link_libraries(${PROJECT_NAME} ${PIG} ${PIGD} raylib vcos brcmGLESv2 brcmEGL pthread rt m bcm_host dl websockets ${LUA54} xnlib-fn ${prof} z robolib)
endmacro()

XN_Build( XNgine src/main.c )
XN_Build( XNgine++ src/main.cpp )
XN_Build( XNgl src/graphics/egl/egl_tests.c )
XN_Build( asmtest src/util/asm/asmtest.c )

XN_Build( Yamsim src/yamsim.cpp)
# find_package(ZLIB)
# XN_Build( fbxloader src/util/fbxloader/fbxparse.c)
# set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -lglfw3 -lopengl32 -lgdi32 -Wall")
