cmake_minimum_required(VERSION 2.6)
project(NimbleInk)

# work around a retarded cmake misconfiguration by default
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -g")
add_definitions(-D_GNU_SOURCE)

add_definitions(-DLOOP_SDL)

find_package(OpenGL REQUIRED)
link_libraries(${OPENGL_LIBRARIES})
include_directories(${OPENGL_INCLUDE_DIR})

find_package(SDL REQUIRED)
link_libraries(${SDL_LIBRARY})
include_directories(${SDL_INCLUDE_DIR})

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

find_package(PNG)
if (PNG_FOUND)
    link_libraries(${PNG_LIBRARY})
    include_directories(${PNG_PNG_INCLUDE_DIR})
    add_definitions(${PNG_DEFINITIONS})
    add_definitions(-DENABLE_LIBPNG)
endif (PNG_FOUND)

find_package(SDL_image)
if (SDLIMAGE_FOUND)
    link_libraries(${SDLIMAGE_LIBRARY})
    include_directories(${SDLIMAGE_INCLUDE_DIR})
    add_definitions(-DENABLE_SDLIMAGE)
endif (SDLIMAGE_FOUND)

add_definitions(-DENABLE_PNM)

set(IMAGE_SOURCES
    image-giflib.c
    image-libjpeg.c
    image-libpng.c
    image-pnm.c
    image-sdl_image.c
)

set(SOURCES
    ${IMAGE_SOURCES}
    archive.c
    archive-unrar.c
    archive-unzip.c
    cpuimage.c
    english.c
    filetype.c
    glimage.c
    glview.c
    loop-common.c
    loop-sdl.c
    reference.c
    stringutils.c
    zipper.c
    inklog.c
)

link_libraries(m)
add_executable(nink ${SOURCES} main.c)
add_executable(nink-imagedebug ${SOURCES} main-imagedebug.c)
add_executable(nink-sorttest english.c main-sorttest.c)

