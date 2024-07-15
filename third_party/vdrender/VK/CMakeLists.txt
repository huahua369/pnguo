set(sources
    GLTFSample.cpp
    GLTFSample.h
    Renderer.cpp
    Renderer.h
	UI.cpp
    UI.h
    stdafx.cpp
    stdafx.h
    "vkrenderer.cpp"
    dpiawarescaling.manifest)

source_group("Sources" FILES ${sources})
source_group("Icon"    FILES ${icon_src}) # defined in top-level CMakeLists.txt

#add_executable(GLTFSample_VK WIN32 ${sources} ${icon_src})
set(sample_lib
    
    Cauldron_VK
    ImGUI
    Vulkan::Vulkan
)
#target_link_libraries(GLTFSample_VK LINK_PUBLIC ${sample_lib})

#set_target_properties(GLTFSample_VK PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d")

add_library(rvk SHARED ${sources} ${icon_src} )
target_link_libraries(rvk LINK_PUBLIC ${sample_lib})
target_include_directories(rvk PRIVATE ${Stb_INCLUDE_DIR})
set_target_properties(rvk PROPERTIES VERSION 1.0 SOVERSION 1)