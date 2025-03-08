# CMakeList.txt: CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.20) 
project ("tcmp")
  
#测试
list(APPEND src_cmp  
    "charts.cpp"  "ecc_sv.cpp"
    "main.cpp" 
    "../rc/resource1.rc"
) 


# 将源代码添加到此项目的可执行文件。
add_executable (tcmp ${src_cmp} ) 

list(APPEND src_vkcmp   
    "main_vk.cpp" 
    "ecc_sv.cpp"
    "mshell.cpp"
    "../rc/resource1.rc"
) 
list(APPEND src_spr   
    "main_spr.cpp" 
    "ecc_sv.cpp"
    "mshell.cpp"
    "../rc/resource1.rc"
) 
list(APPEND src_mtl   
    "main_tl.cpp" 
    "ecc_sv.cpp"
    "mshell.cpp"
    "../rc/resource1.rc"
    "../njson.natvis"
) 


list(APPEND src_hex   
    "main_hex.cpp" 
    "ecc_sv.cpp"
    "mshell.cpp"
    "SDL3_ttf/src/SDL_ttf.c"
    "SDL3_ttf/src/SDL_hashtable.c"
    "SDL3_ttf/src/SDL_hashtable_ttf.c"
    "SDL3_ttf/src/SDL_renderer_textengine.c"
    "SDL3_ttf/src/SDL_surface_textengine.c"
    "SDL3_ttf/src/SDL_gpu_textengine.c"
    "../rc/resource1.rc"
    "../njson.natvis"
) 


# 将源代码添加到此项目的可执行文件。
add_executable (vkcmp ${src_vkcmp} ) 
add_executable (vkr_spr ${src_spr} ) 
add_executable (mtl ${src_mtl} ) 
add_executable (hex ${src_hex} ) 

#include_directories("${SDL3DIR_ROOT}/include")
#list(APPEND CMAKE_PREFIX_PATH "${SDL3DIR_ROOT}/cmake")
#find_package(SDL3 CONFIG REQUIRED)

#message("stb路径 ${Stb_INCLUDE_DIR}")
find_package(OpenSSL REQUIRED)

find_package(Libssh2 CONFIG REQUIRED)
       
#find_package(libuv CONFIG REQUIRED)
 
find_package(OpenCV CONFIG REQUIRED)
#find_package(unofficial-joltphysics CONFIG REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(hwloc REQUIRED IMPORTED_TARGET hwloc)

find_package(cmark CONFIG REQUIRED)
find_package(md4c CONFIG REQUIRED)

set(lklib)
list(APPEND lklib  
    ZLIB::ZLIB
    Vulkan::Vulkan 
    Vulkan::Headers GPUOpen::VulkanMemoryAllocator
    PkgConfig::LIBRSVG
    PkgConfig::hwloc
    glm::glm-header-only
    nlohmann_json::nlohmann_json
    OpenSSL::SSL OpenSSL::Crypto
    #${SDL3_STATIC_LIBRARIES}
    #${SDL3_LIBRARIES}
   
    harfbuzz::harfbuzz 
    #$<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    #opencv_ml opencv_dnn opencv_core opencv_flann opencv_features2d opencv_photo opencv_highgui
    pnguo
    tpv
    libssh2::libssh2
    CURL::libcurl
    
    cmark::cmark md4c::md4c
    #$<IF:$<TARGET_EXISTS:Libssh2::libssh2_shared>,Libssh2::libssh2_shared,Libssh2::libssh2_static>
   # rdx12
   # unofficial::joltphysics::Jolt
)
if(CMAKE_HOST_WIN32)
list(APPEND lklib  
UxTheme
)
endif() 
include_directories(tcmp PRIVATE ${Stb_INCLUDE_DIR} ${TINYGLTF_INCLUDE_DIRS})
target_link_libraries(tcmp PRIVATE ${lklib})
 
target_link_libraries(vkcmp PRIVATE ${lklib} ) 
target_link_libraries(vkr_spr PRIVATE ${lklib} ) 
target_link_libraries(mtl PRIVATE ${lklib}  )
target_link_libraries(hex PRIVATE ${lklib}  )
 
#复制测试文本
file(COPY sh938.txt DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ed.json DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY blender_icons.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY button20.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY button21.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
  
