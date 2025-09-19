include(${CMAKE_CURRENT_SOURCE_DIR}/common.cmake)

set(pnguo_src
"mapView.cpp"
"tinysdl3.cpp"
"pnguo.cpp"
"tria.cpp"
"backend_2d.cpp"
"font_core.cpp"
"mnet.cpp"
"buffer.cpp"
"win_core.cpp" 
"page.cpp"  
"vkrenderer.cpp"  
"testshader.cpp"  
"audio.cpp"  
)
 
set(Shaders_PostProc_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blend.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blur.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/DownSamplePS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemappers.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemapping.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAASharpenerCS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAA.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAASharpenerCS.glsl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TAA.glsl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemappingCS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ColorConversionPS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/MagnifierPS.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/base3d.vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/base3d.frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/oit_blendCS.glsl
)

set(Shaders_GLTF_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTF_VS2PS_IO.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFMotionVectorsPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFMotionVectorsPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky.v.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/skinning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/pbr_px.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFVertexFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/perFrameStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/PBRTextures.h 
)
set(Images_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/brdfLut.dds 
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/diffuse.dds 
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/specular.dds 
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/default_specular.dds 
)


find_package(FLAC CONFIG REQUIRED)
find_package(mpg123 CONFIG REQUIRED)
find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(FFTW3l CONFIG REQUIRED)

add_library (pnguo STATIC ${pnguo_src})
#add_library (pnguo SHARED ${pnguo_src})

target_link_libraries (pnguo PkgConfig::LIBRSVG
#Vulkan::Vulkan Vulkan::Headers
GPUOpen::VulkanMemoryAllocator glm::glm-header-only nlohmann_json::nlohmann_json 
    PkgConfig::Clipper2  PkgConfig::Clipper2Z 
    tpv
    SDL3::SDL3
    dxcompiler
    FLAC::FLAC MPG123::libmpg123
    FFTW3::fftw3 FFTW3::fftw3f FFTW3::fftw3l
)
target_include_directories (pnguo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) 
copyTargetCommand("${Shaders_GLTF_src}" ${EXECUTABLE_OUTPUT_PATH}/ShaderLibVK copied_vk_shaders_gltf_src)
copyTargetCommand("${Shaders_PostProc_src}" ${EXECUTABLE_OUTPUT_PATH}/ShaderLibVK copied_vk_shaders_postproc_src)
copyTargetCommand("${Images_src}" ${EXECUTABLE_OUTPUT_PATH}/images copied_Images_src)
add_dependencies(pnguo copied_vk_shaders_gltf_src copied_vk_shaders_postproc_src copied_Images_src)

 