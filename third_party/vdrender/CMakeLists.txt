set(sources
    main.cpp
)

source_group("Sources" FILES ${sources})
source_group("Icon"    FILES ${icon_src}) # defined in top-level CMakeLists.txt

add_executable(GLTFSample WIN32 ${sources} ${icon_src})

target_link_libraries(GLTFSample LINK_PUBLIC rvk rdx12)

set_target_properties(GLTFSample PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d")
