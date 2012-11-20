cmake_minimum_required(VERSION 2.6)
project(NimbleInk)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -std=c99 -Wall")
add_definitions(-D_GNU_SOURCE)

find_package(OpenGL REQUIRED)
link_libraries(${OPENGL_LIBRARIES})
include_directories(${OPENGL_INCLUDE_DIR})

find_package(Lua51 REQUIRED)
link_libraries(${LUA_LIBRARIES})
include_directories(${LUA_INCLUDE_DIR})

find_package(JPEG)
if (JPEG_FOUND)
    link_libraries(${JPEG_LIBRARIES})
    include_directories(${JPEG_INCLUDE_DIR})
    add_definitions(-DENABLE_LIBJPEG)
endif (JPEG_FOUND)

find_package(GIF)
if (GIF_FOUND)
    link_libraries(${GIF_LIBRARIES})
    include_directories(${GIF_INCLUDE_DIR})
    add_definitions(-DENABLE_GIFLIB)
endif (GIF_FOUND)

find_package(SDL_image)
if (SDLIMAGE_FOUND)
    link_libraries(${SDLIMAGE_LIBRARY})
    include_directories(${SDLIMAGE_INCLUDE_DIR})
    add_definitions(-DENABLE_SDLIMAGE)
endif (SDLIMAGE_FOUND)

add_definitions(-DENABLE_PNM)

set(IMAGE_SOURCES
#    image-giflib.c
#    image-libjpeg.c
#    image-pnm.c
#    image-sdl_image.c
)

set(SOURCES
    ${IMAGE_SOURCES}
    lua-bind.c
)

link_libraries(m)
add_executable(nink ${SOURCES} main.c)

