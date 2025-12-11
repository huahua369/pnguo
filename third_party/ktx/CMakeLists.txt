# Copyright 2015-2025 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/modules/")

# N.B. When used before the project() command, PROJECT_IS_TOP_LEVEL will not
# do what you likely at first expect. It will be TRUE if this is included
# in another project.
 
project(libktx)

if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS" OR CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set( APPLE_LOCKED_OS ON )
    else()
        set( APPLE_MAC_OS ON )
    endif()
endif()
 
# OPTIONS

option( LIBKTX_VERSION_READ_ONLY "Build the read only library" OFF)
option( LIBKTX_VERSION_FULL "Build the full library" ON)

option( LIBKTX_FEATURE_KTX1 "Enable KTX 1 support." ON )
# TODO: Remove this option?
option( LIBKTX_FEATURE_KTX2 "Enable KTX 2 support." ON )
option( LIBKTX_FEATURE_VK_UPLOAD "Enable Vulkan texture upload." ON )
option( LIBKTX_FEATURE_GL_UPLOAD "Enable OpenGL texture upload." OFF )
option( LIBKTX_FEATURE_ETC_UNPACK "ETC decoding support." ON )
 

include_directories("include")
include_directories("external")
include_directories("basisu")
include_directories("zstd")
include_directories("utils")
# Platform specific settings

set(bitness 64)
 

if(NOT BUILD_SHARED_LIBS)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

if(MINGW)
    # Check if the Threads package is provided; if using Mingw it MIGHT be
    find_package(Threads)
elseif(LINUX)
    find_package(Threads REQUIRED)
endif()

# Source Lists
set(LIBKTX_MAIN_SRC
    src/texture.c
    src/texture1.c
    src/texture2.c
    src/hashlist.c
    src/checkheader.c
    src/swap.c
    src/memstream.c
    src/filestream.c
    src/miniz_wrapper.cpp
    zstd/zstd.c
    #zstd/zstddeclib.c
    external/dfdutils/createdfd.c
    external/dfdutils/colourspaces.c
    external/dfdutils/dfd.h
    external/dfdutils/interpretdfd.c
    external/dfdutils/printdfd.c
    external/dfdutils/queries.c
    external/dfdutils/vk2dfd.c
    external/dfdutils/vk2dfd.inl
    src/vk_format.h
    src/vkFormat2glFormat.inl
    src/vkFormat2glInternalFormat.inl
    src/vkFormat2glType.inl
    src/vkformat_check.c
    src/vkformat_check_variant.c
    src/vkformat_enum.h
    src/vkformat_str.c
    src/vkformat_typesize.c
)

set(LIBKTX_MAIN_SRCa
    include/KHR/khr_df.h
    include/ktx.h
    src/astc_codec.cpp
    src/basis_sgd.h
    src/basis_transcode.cpp
    src/miniz_wrapper.cpp
    src/checkheader.c
    external/dfdutils/createdfd.c
    external/dfdutils/colourspaces.c
    external/dfdutils/dfd.h
    external/dfdutils/interpretdfd.c
    external/dfdutils/printdfd.c
    external/dfdutils/queries.c
    external/dfdutils/vk2dfd.c
    external/dfdutils/vk2dfd.inl
    external/dfdutils/vulkan/vk_platform.h
    external/dfdutils/vulkan/vulkan_core.h
    src/etcunpack.cxx
    src/filestream.c
    src/filestream.h
    src/formatsize.h
    src/gl_format.h
    src/glformat_str.c
    src/hashlist.c
    src/info.c
    src/ktxint.h
    src/memstream.c
    src/memstream.h
    src/strings.c
    src/swap.c
    src/texture.c
    src/texture.h
    src/texture2.c
    src/texture2.h
    src/texture_funcs.inl
    src/uthash.h
    src/vk2gl.h
    src/vk_format.h
    src/vkFormat2glFormat.inl
    src/vkFormat2glInternalFormat.inl
    src/vkFormat2glType.inl
    src/vkformat_check.c
    src/vkformat_check_variant.c
    src/vkformat_enum.h
    src/vkformat_str.c
    src/vkformat_typesize.c
    utils/unused.h

)

if(LIBKTX_FEATURE_VK_UPLOAD)
    list( APPEND LIBKTX_MAIN_SRC 
            src/vk_funcs.c
            src/vk_funcs.h
            src/vkloader.c )
endif()

if (LIBKTX_FEATURE_ETC_UNPACK)
    list( APPEND LIBKTX_MAIN_SRC external/etcdec/etcdec.cxx )
endif()
 
#add_library(mktx SHARED ${LIBKTX_MAIN_SRC})
add_library(mktx STATIC ${LIBKTX_MAIN_SRC})
target_link_libraries(mktx PRIVATE Vulkan::Vulkan Vulkan::Headers)