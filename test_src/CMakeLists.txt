# CMakeList.txt: CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.20) 
project ("tcmp")
  
#测试
list(APPEND src_cmp  
    "charts.cpp"  "../src/pnguo/ecc_sv.cpp"
    "main.cpp" 
    "../rc/resource1.rc"
) 


# 将源代码添加到此项目的可执行文件。
#add_executable (tcmp ${src_cmp} ) 

list(APPEND src_vkcmp   
    "main_vk.cpp" 
    "../src/pnguo/ecc_sv.cpp"
    "../src/pnguo/plot.cpp"
    "../third_party/spine/spine-sdl3/spinesdl3.cpp" 
    "mshell.cpp"
    "../rc/resource1.rc"
) 
list(APPEND src_spr   
    "main_spr.cpp" 
    "../src/pnguo/ecc_sv.cpp"
    "mshell.cpp"
    "../rc/resource1.rc"
) 
list(APPEND src_mtl   
    "main_tl.cpp" 
    "../src/pnguo/ecc_sv.cpp"
    "mshell.cpp" 
    "logic_gates.cpp" 
    "../src/pnguo/editor_2d.cpp"
    "../third_party/spine/spine-sdl3/spinesdl3.cpp" 
    "../third_lib/SDL/src/test/SDL_test_common.c"
    "../third_lib/SDL/src/test/SDL_test_font.c"
    "../third_lib/SDL/src/test/SDL_test_memory.c"
    "../third_lib/SDL/src/test/SDL_test_crc32.c"
    "../third_lib/SDL/src/test/SDL_test_assert.c"
    "../third_lib/SDL/src/test/SDL_test_fuzzer.c"
    "../third_lib/SDL/src/test/SDL_test_log.c"
    "../third_lib/SDL/src/test/SDL_test_md5.c"
    "../third_lib/SDL/src/test/SDL_test_harness.c"
    "../third_lib/SDL/src/test/SDL_test_compare.c"
    "../rc/resource1.rc"
    "../njson.natvis"
) 
list(APPEND src_mw
    "../src/pnguo/ecc_sv.cpp"
    "minesweeper.cpp"
    "../src/pnguo/editor_2d.cpp"
    "../third_party/spine/spine-sdl3/spinesdl3.cpp" 
    "../rc/resource1.rc"
    "../njson.natvis"
) 
list(APPEND src_audios 
    "main_testaudiio.cpp"  
) 
list(APPEND src_mmd   
    "main_markdown.cpp" 
    "../src/pnguo/ecc_sv.cpp"
    "mshell.cpp" 
    "../rc/resource1.rc"
    "../njson.natvis"
) 
list(APPEND src_vkvg   
    "main_vg.cpp" 
    ${VKH_SRC} 
    "../third_party/tinyspline/parson.c"
    "../third_party/tinyspline/tinyspline.c"
    "../third_party/tinyspline/tinysplinecxx.cpp"  
    "../src/pnguo/audio.cpp"
    "../src/pnguo/mapView.cpp"
    "../src/pnguo/vg.cpp"
    "../src/pnguo/pnguo.cpp"
    "../src/pnguo/render.cpp"
    "../src/pnguo/font_core.cpp"
    "../src/pnguo/tinysdl3.cpp"
    "../src/pnguo/vkrenderer.cpp"
    "../src/pnguo/gpu_vk.cpp"
    "../src/pnguo/win_core.cpp"
    "../src/pnguo/gui.cpp"
    "../src/pnguo/app.cpp"
    "../third_party/vkvg/vkvg_cx.cpp"
    "../third_party/stb_src/stb_src.c" 
    "../third_party/mcut/bvh.cpp"  
    "../third_party/mcut/frontend.cpp"  
    "../third_party/mcut/hmesh.cpp"  
    "../third_party/mcut/kernel.cpp"  
    "../third_party/mcut/math.cpp"  
    "../third_party/mcut/preproc.cpp"  
    "../third_party/mcut/shewchuk.c"  
    "../third_party/mcut/mcut.cpp"  
    "../third_party/mcut/stlrw.cpp"  
    "../third_party/mcut/obj.c"
    "../third_party/mcut/mcut_cx.cpp"  
    "../rc/resource1.rc"
    "../njson.natvis"
) 
message("vkvgtest")
message(${VKH_SRC})


# 将源代码添加到此项目的可执行文件。
add_executable (vkcmp ${src_vkcmp} ) 
#add_executable (vkr_spr ${src_spr} ) 
add_executable (mtl ${src_mtl} ) 
add_executable (audios ${src_audios} ) 
#add_executable (mmd ${src_mmd} ) 
add_executable (mw ${src_mw} ) 
add_executable (vgtest ${src_vkvg} ) 
#管理员权限
#SET_TARGET_PROPERTIES(mtl PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
#include_directories("${SDL3DIR_ROOT}/include")
#list(APPEND CMAKE_PREFIX_PATH "${SDL3DIR_ROOT}/cmake")
#find_package(SDL3 CONFIG REQUIRED)

#message("stb路径 ${Stb_INCLUDE_DIR}")
find_package(OpenSSL REQUIRED)

find_package(Libssh2 CONFIG REQUIRED)
find_package(cpuid CONFIG REQUIRED)
       
find_package(libuv CONFIG REQUIRED)
 
#find_package(OpenCV CONFIG REQUIRED)
#find_package(unofficial-joltphysics CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(libzip CONFIG REQUIRED)
#pkg_check_modules(hwloc REQUIRED IMPORTED_TARGET hwloc)
 find_package(unofficial-noise CONFIG REQUIRED)
 find_package(unofficial-noiseutils CONFIG REQUIRED)
#find_package(cmark CONFIG REQUIRED)  

#git submodule add https://github.com/EsotericSoftware/spine-runtimes third_lib/spine
 

set(lklib)
list(APPEND lklib  
    Dwmapi
    libzip::zip
    ZLIB::ZLIB
    #Vulkan::Vulkan 
    #Vulkan::Headers 
    GPUOpen::VulkanMemoryAllocator
   # PkgConfig::LIBRSVG
   # PkgConfig::hwloc
    glm::glm-header-only
    nlohmann_json::nlohmann_json
    OpenSSL::SSL OpenSSL::Crypto
    #${SDL3_STATIC_LIBRARIES}
    #${SDL3_LIBRARIES}
   unofficial::noise::noise-static
   unofficial::noiseutils::noiseutils-static
   # harfbuzz::harfbuzz 
    harfbuzz::harfbuzz-raster 
    #harfbuzz::harfbuzz-gpu
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    #opencv_ml opencv_dnn opencv_core opencv_flann opencv_features2d opencv_photo opencv_highgui
    pnguo
    tpv
    libssh2::libssh2
    CURL::libcurl
    spine-c  
    cpuid::cpuid
    #libsoundio_static
    #cmark::cmark 
    #md4c::md4c
    #$<IF:$<TARGET_EXISTS:Libssh2::libssh2_shared>,Libssh2::libssh2_shared,Libssh2::libssh2_static>
   # rdx12
   # unofficial::joltphysics::Jolt 
)
if(CMAKE_HOST_WIN32)
list(APPEND lklib  
UxTheme
)
endif() 
add_definitions(-DSOUNDIO_STATIC_LIBRARY=1)
#find_package(libsoundio CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)

find_package(FLAC CONFIG REQUIRED)
find_package(mpg123 CONFIG REQUIRED)
find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(FFTW3l CONFIG REQUIRED)

list(APPEND lkalib  
#libsoundio::libsoundio
Ogg::ogg
)
list(APPEND vkvglib  
 glm::glm-header-only
 nlohmann_json::nlohmann_json
 Fontconfig::Fontconfig
 Vulkan::Vulkan
 SDL3::SDL3
 PkgConfig::Clipper2  PkgConfig::Clipper2Z 
 harfbuzz::harfbuzz-raster
 libzip::zip
 ZLIB::ZLIB
 dxcompiler
 draco::draco
 FFTW3::fftw3 FFTW3::fftw3f FFTW3::fftw3l
 FLAC::FLAC MPG123::libmpg123
 glutess
)
#include_directories(tcmp PRIVATE ${Stb_INCLUDE_DIR} ${TINYGLTF_INCLUDE_DIRS})
#target_link_libraries(tcmp PRIVATE ${lklib})
 
target_link_libraries(vkcmp PRIVATE ${lklib} ) 
#target_link_libraries(vkr_spr PRIVATE ${lklib} ) 
target_link_libraries(mtl PRIVATE ${lklib}  )
target_link_libraries(mw PRIVATE ${lklib} )
target_link_libraries(audios PRIVATE ${lkalib})

target_link_libraries(vgtest PRIVATE ${vkvglib})
#target_link_libraries(mmd PRIVATE ${lklib}  )  
 
#复制测试文本
file(COPY sh938.txt DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ed.json DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY blender_icons.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY button20.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY button21.svg DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY mw2.png DESTINATION ${EXECUTABLE_OUTPUT_PATH}/data)
file(COPY xh1.png DESTINATION ${EXECUTABLE_OUTPUT_PATH}/data)
file(COPY xh1.atlas DESTINATION ${EXECUTABLE_OUTPUT_PATH}/data)
file(COPY xh1.json DESTINATION ${EXECUTABLE_OUTPUT_PATH}/data)
file(COPY seguiemj.ttf DESTINATION ${EXECUTABLE_OUTPUT_PATH}/data)
file(COPY mari.png DESTINATION ${EXECUTABLE_OUTPUT_PATH}/data)
  