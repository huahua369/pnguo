# CMakeList.txt: CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
set(VCPKG_TARGET_TRIPLET x64-windows-static)
set(CMAKE_TOOLCHAIN_FILE "E:/sdk/vcpkg/scripts/buildsystems/vcpkg.cmake")


IF(WIN32)
set(CMAKE_CXX_STANDARD 23)
else()
set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_EXTENSIONS ON)

project ("pnguo")

message("msvc: ${CMAKE_SYSTEM_VERSION}")
message("CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

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
set(SDL3DIR_ROOT "E:/sdk/SDL3/out/install/x64-${CBT}")
set(SDL3_DIR "E:/sdk/SDL3/out/install/x64-${CBT}/cmake")
message("sdl ${SDL3_DIR}")
message("stb路径 ${Stb_DIR}")
message("type--${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd") 
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT") 
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT") 
 
#设置库文件输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/bin/${platform_dir})
#设置执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
  
#vkvg 
add_definitions(-DVKVG_RECORDING)
add_definitions(-DVKH_USE_VMA)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
FILE(GLOB VKVG_SRC third_party/vkvg/src/*.c third_party/vkvg/src/deps/*.c third_party/vkvg/src/recording/*.c
  third_party/vkh/src/*.c third_party/vkh/src/vma/*.cpp 
)
SET (VKVG_PUBLIC_HEADERS "third_party/vkvg/vkvg.h;third_party/vkvg/vkvg-svg.h")

list(APPEND src_cmp    
"third_party/stb_src/stb_src.c" 
"third_party/vkgui/mapView.cpp"
"third_party/vkgui/tinysdl3.cpp"
"third_party/vkgui/pnguo.cpp"
"third_party/vkgui/buffer.cpp"
"third_party/vkgui/win_core.cpp" 
"third_party/vkgui/page.cpp"  
"third_party/tinyspline/parson.c"
"third_party/tinyspline/tinyspline.c"
"third_party/tinyspline/tinysplinecxx.cpp" 
"third_party/vkvg/vkvgcx.cpp"  
"third_party/vkgui/vkrenderer.cpp"  

"third_party/mcut/bvh.cpp"  
"third_party/mcut/frontend.cpp"  
"third_party/mcut/hmesh.cpp"  
"third_party/mcut/kernel.cpp"  
"third_party/mcut/math.cpp"  
"third_party/mcut/preproc.cpp"  
"third_party/mcut/shewchuk.c"  
"third_party/mcut/mcut.cpp"  
"third_party/mcut/mcut_cx.cpp"  
"third_party/mcut/stlrw.cpp"  
"third_party/mcut/obj.c"
)

#测试
list(APPEND src_cmp  
    ${VKVG_SRC} 
    "test_src/charts.cpp"
    "test_src/main.cpp"
    "rc/resource1.rc"
) 

# 将源代码添加到此项目的可执行文件。
add_executable (tcmp ${src_cmp}  ) 

include_directories("third_party") 
include_directories("third_party/vkgui") 
include_directories("third_party/mcut") 

include_directories("third_party/vkvg")
include_directories("third_party/vkvg/src")
include_directories("third_party/vkvg/src/recording")
include_directories("third_party/vkh/include")
include_directories("third_party/vkh/src")

include_directories("${SDL3DIR_ROOT}/include")
list(APPEND CMAKE_PREFIX_PATH "${SDL3DIR_ROOT}/cmake")
find_package(SDL3 CONFIG REQUIRED)

#message("stb路径 ${Stb_INCLUDE_DIR}")
find_package(OpenSSL REQUIRED)
 
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
#find_package(NanoSVG CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Vulkan REQUIRED) 
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp")
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
#find_package(GEOS CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
    pkg_check_modules(Clipper2 REQUIRED IMPORTED_TARGET Clipper2)
    pkg_check_modules(Clipper2Z REQUIRED IMPORTED_TARGET Clipper2Z)
       
find_package(libuv CONFIG REQUIRED)
pkg_check_modules(LIBRSVG librsvg-2.0 IMPORTED_TARGET REQUIRED)
pkg_check_modules(GTHREAD gthread-2.0 IMPORTED_TARGET REQUIRED)

find_package(EnTT CONFIG REQUIRED) 
find_package(OpenCV CONFIG REQUIRED)
#find_package(unofficial-joltphysics CONFIG REQUIRED)

set(lklib)
list(APPEND lklib  
    ZLIB::ZLIB
    Vulkan::Vulkan 
    Vulkan::Headers GPUOpen::VulkanMemoryAllocator
    PkgConfig::LIBRSVG
    glm::glm-header-only
    nlohmann_json::nlohmann_json
    OpenSSL::SSL OpenSSL::Crypto
    #${SDL3_STATIC_LIBRARIES}
    ${SDL3_LIBRARIES}
    harfbuzz::harfbuzz 
    PkgConfig::Clipper2  PkgConfig::Clipper2Z 
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    opencv_ml opencv_dnn opencv_core opencv_flann opencv_features2d opencv_photo opencv_highgui
   # rvk
   # rdx12
   # unofficial::joltphysics::Jolt
)
target_include_directories(tcmp PRIVATE ${Stb_INCLUDE_DIR} ${TINYGLTF_INCLUDE_DIRS})
target_link_libraries(tcmp PRIVATE ${lklib})
 
#复制测试文本
file(COPY test_src/sh938.txt DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY test_src/ed.json DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY test_src/blender_icons.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY test_src/button20.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY test_src/button21.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})

#3d渲染器
#add_definitions(-DTINYGLTF_NO_INCLUDE_STB_IMAGE)
#add_definitions(-DTINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE)

add_subdirectory(third_party/cauldron)
add_subdirectory(third_party/vdrender/DX12)
add_subdirectory(third_party/vdrender/VK)

# TODO: 如有需要，请添加测试并安装目标。
