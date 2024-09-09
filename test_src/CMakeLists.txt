# CMakeList.txt: CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8) 
project ("tcmp")
  
#测试
list(APPEND src_cmp  
    "charts.cpp"
    "main.cpp" 
    "../rc/resource1.rc"
) 


# 将源代码添加到此项目的可执行文件。
add_executable (tcmp ${src_cmp}   "ecc_sv.cpp") 

#include_directories("${SDL3DIR_ROOT}/include")
#list(APPEND CMAKE_PREFIX_PATH "${SDL3DIR_ROOT}/cmake")
#find_package(SDL3 CONFIG REQUIRED)

#message("stb路径 ${Stb_INCLUDE_DIR}")
find_package(OpenSSL REQUIRED)
 
       
find_package(libuv CONFIG REQUIRED)
 
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
    #${SDL3_LIBRARIES}
   
    harfbuzz::harfbuzz 
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    opencv_ml opencv_dnn opencv_core opencv_flann opencv_features2d opencv_photo opencv_highgui
    vkgui
    tpv
   # rdx12
   # unofficial::joltphysics::Jolt
)
target_include_directories(tcmp PRIVATE ${Stb_INCLUDE_DIR} ${TINYGLTF_INCLUDE_DIRS})
target_link_libraries(tcmp PRIVATE ${lklib})
 
#复制测试文本
file(COPY sh938.txt DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ed.json DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY blender_icons.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY button20.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY button21.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
 