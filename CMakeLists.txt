# CMakeList.txt: CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
 
IF(WIN32)
set(CMAKE_CXX_STANDARD 23)
else()
set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_EXTENSIONS ON)

project ("pnguo")

message("VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
if(VCPKG_TARGET_TRIPLET STREQUAL "x64-windows")
set(VCPKG_TARGET_TRIPLET x64-windows-static)
endif()
if(VCPKG_TARGET_TRIPLET STREQUAL "x86-windows")
set(VCPKG_TARGET_TRIPLET x86-windows-static)
endif()
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

message("msvc: ${CMAKE_SYSTEM_VERSION}")
message("CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message("VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(platform_i)
    set(platform_dir x64)
ELSE()
    set(platform_i 32)
    set(platform_dir x86)
ENDIF()

set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}) 

if(NOT DEFINED CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Release")
endif()

set(CBT "${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE MATCHES "Debug")
set(platform_dir "${platform_dir}d")
else()
set(CBT "Release")
endif() 

message("stb路径 ${Stb_DIR}")
message("type--${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd") 
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT") 
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT") 
 
#设置执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/bin/${platform_dir})
#设置库文件输出目录
set(LIBRARY_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})
   
#第三方库
if(NOT DEFINED SDL3DIR_ROOT)
set(SDL_STATIC ON)
set(SDL_SHARED OFF) 
set(SDL_DUMMYCAMERA OFF)
set(SDL_DUMMYAUDIO OFF)
set(SDL_DISKAUDIO OFF)
set(SDL_DUMMYVIDEO OFF)
set(SDL_LIBC ON)
set(SDL_WASAPI ON)
set(SDL_RENDER_METAL OFF)
set(SDL_OPENGL OFF)
set(SDL_OPENGLES OFF)
set(SDL_DIRECTX ON)
set(SDL_RENDER_D3D OFF)
set(SDL_RENDER_D3D11 ON)
set(SDL_RENDER_D3D12 ON)
set(SDL_RENDER_GPU ON)
set(SDL_GPU ON)
set(SDL_RENDER_VULKAN ON)  

add_subdirectory(third_lib/SDL)
else()
#设置这两个变量或自定义find路径
#set(SDL3DIR_ROOT "E:/sdk/SDL3/out/install/x64-${CBT}")
#set(SDL3_DIR "${SDL3DIR_ROOT}/cmake")
#message("sdl ${SDL3_DIR}")
find_package(SDL3 REQUIRED) 
endif()

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DVKVG_RECORDING)

add_definitions(-DVKH_USE_VMA)
add_definitions(-DXATLAS_C_API=1)

#find_package(mimalloc CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED) 
find_package(nlohmann_json REQUIRED)
find_package(Vulkan REQUIRED) 
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp")
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h") 
find_package(EnTT CONFIG REQUIRED)  


find_package(PkgConfig REQUIRED)
pkg_check_modules(Clipper2 REQUIRED IMPORTED_TARGET Clipper2)
pkg_check_modules(Clipper2Z REQUIRED IMPORTED_TARGET Clipper2Z)
pkg_check_modules(LIBRSVG librsvg-2.0 IMPORTED_TARGET REQUIRED)
pkg_check_modules(GTHREAD gthread-2.0 IMPORTED_TARGET REQUIRED)  
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2) 
#vkvg 
FILE(GLOB VKVG_SRC third_party/vkvg/src/*.c third_party/vkvg/src/deps/*.c third_party/vkvg/src/recording/*.c
  third_party/vkh/src/*.c third_party/vkh/src/vma/*.cpp 
)
SET (VKVG_PUBLIC_HEADERS "third_party/vkvg/vkvg.h;third_party/vkvg/vkvg-svg.h")

list(APPEND tpv_src    
 ${VKVG_SRC} 
"third_party/stb_src/stb_src.c" 

"third_party/tinyspline/parson.c"
"third_party/tinyspline/tinyspline.c"
"third_party/tinyspline/tinysplinecxx.cpp" 
"third_party/vkvg/vkvgcx.cpp"  
"src/vkgui/mapView.cpp"  

"third_party/mcut/bvh.cpp"  
"third_party/mcut/frontend.cpp"  
"third_party/mcut/hmesh.cpp"  
"third_party/mcut/kernel.cpp"  
"third_party/mcut/math.cpp"  
"third_party/mcut/preproc.cpp"  
"third_party/mcut/shewchuk.c"  
"third_party/mcut/mcut.cpp"  
"third_party/mcut/stlrw.cpp"  
"third_party/mcut/obj.c"
"third_party/mcut/mcut_cx.cpp"  
"third_party/xatlas/xatlas.cpp"
)

include_directories("third_party") 
include_directories("src") 
include_directories("src/vkgui") 
include_directories("third_party/mcut") 
include_directories("third_party/vkvg")
include_directories("third_party/vkvg/src")
include_directories("third_party/vkvg/src/recording")
include_directories("third_party/vkh/include")
include_directories("third_party/vkh/src")

add_library (tpv STATIC ${tpv_src})
#add_library (tpv SHARED ${tpv_src})
target_include_directories(tpv PRIVATE ${Stb_INCLUDE_DIR} ${TINYGLTF_INCLUDE_DIRS})
target_link_libraries(tpv PRIVATE Vulkan::Vulkan Vulkan::Headers GPUOpen::VulkanMemoryAllocator glm::glm-header-only nlohmann_json::nlohmann_json
PkgConfig::GTHREAD
#mimalloc-static 
PkgConfig::LIBGIT2
)

#3d渲染器
add_subdirectory(src/vkgui)
#add_definitions(-DTINYGLTF_NO_INCLUDE_STB_IMAGE)
#add_definitions(-DTINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE)

add_subdirectory(test_src)
add_dependencies(vkgui tpv)
add_dependencies(tcmp vkgui)
# TODO: 如有需要，请添加测试并安装目标。

list(APPEND snake_src     
#"third_lib/SDL/examples/game/01-snake/main.c" 
"third_lib/SDL/examples/demo/01-snake/snake.c" 
)
add_executable (snake ${snake_src}  ) 
target_link_libraries(snake PRIVATE SDL3::SDL3 mimalloc-static
)