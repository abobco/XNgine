# Install script for directory: D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/raylib")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/libwebsockets.pc")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/libwebsockets_static.pc")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdev_headersx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/include/libwebsockets")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibrariesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/lib/libwebsockets.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/include/libwebsockets.h"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/lws_config.h"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/plugins/ssh-base/include/lws-plugin-ssh.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibrariesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/lib/libwebsockets.so.16")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibrariesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/lib/libwebsockets.so")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/include/libwebsockets.h"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/lws_config.h"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/plugins/ssh-base/include/lws-plugin-ssh.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/bin/libwebsockets-test-server")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/bin/libwebsockets-test-server-extpoll")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/bin/libwebsockets-test-lejp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/bin/libwebsockets-test-client")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/libwebsockets-test-server" TYPE FILE FILES
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/libwebsockets-test-server.key.pem"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/libwebsockets-test-server.pem"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/favicon.ico"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/leaf.jpg"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/candide.zip"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/libwebsockets.org-logo.svg"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/http2.png"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/wss-over-h2.png"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/lws-common.js"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/test.html"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/test.css"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/test.js"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/libwebsockets-test-server/private" TYPE FILE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/private/index.html")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xexamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/libwebsockets-test-server" TYPE FILE FILES
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/lws-ssh-test-keys"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-src/test-apps/lws-ssh-test-keys.pub"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets" TYPE FILE FILES
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/CMakeFiles/LibwebsocketsConfig.cmake"
    "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/LibwebsocketsConfigVersion.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets/LibwebsocketsTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets/LibwebsocketsTargets.cmake"
         "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/CMakeFiles/Export/lib/cmake/libwebsockets/LibwebsocketsTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets/LibwebsocketsTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets/LibwebsocketsTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets" TYPE FILE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/CMakeFiles/Export/lib/cmake/libwebsockets/LibwebsocketsTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libwebsockets" TYPE FILE FILES "D:/Code/C++/raylib/XNgine/build/_deps/libwebsockets-build/CMakeFiles/Export/lib/cmake/libwebsockets/LibwebsocketsTargets-debug.cmake")
  endif()
endif()

