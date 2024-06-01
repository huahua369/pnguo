set(sources
    GLTFSample.cpp
    GLTFSample.h
    Renderer.cpp
    Renderer.h
    UI.cpp
    UI.h
    stdafx.cpp
    stdafx.h
    dpiawarescaling.manifest)

source_group("Sources" FILES ${sources})
source_group("Icon"    FILES ${icon_src}) # defined in top-level CMakeLists.txt

#add_executable(GLTFSample_DX12 WIN32 ${sources} ${common} ${icon_src})
#target_link_libraries(GLTFSample_DX12 LINK_PUBLIC Cauldron_DX12 ImGUI amd_ags d3dcompiler D3D12)

#set_target_properties(GLTFSample_DX12 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d")

add_library(rdx12 SHARED ${sources} ${common} ${icon_src})
target_link_libraries(rdx12 LINK_PUBLIC Cauldron_DX12 ImGUI amd_ags d3dcompiler D3D12)
target_include_directories(rdx12 PRIVATE ${Stb_INCLUDE_DIR})
set_target_properties(rdx12 PROPERTIES VERSION 1.0 SOVERSION 1)