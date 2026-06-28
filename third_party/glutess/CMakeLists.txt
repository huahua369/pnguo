CMAKE_MINIMUM_REQUIRED(VERSION 3.13)

SET(LANG "C")
SET(CMAKE_${LANG}_STANDARD 11)

PROJECT(glutess VERSION 0.1.0 DESCRIPTION "GLU Tessellator library" LANGUAGES ${LANG})

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

FILE(GLOB sources src/*.c)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS (-DDEBUG)
ENDIF ()

IF (UNIX)
    ADD_COMPILE_OPTIONS(-lm)
ENDIF()


ADD_LIBRARY("${PROJECT_NAME}" STATIC ${sources})
SET_TARGET_PROPERTIES("${PROJECT_NAME}" PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        C_STANDARD 11
        C_EXTENSIONS OFF
        PUBLIC_HEADER include/glutess.h
        LINKER_LANGUAGE C
)

TARGET_INCLUDE_DIRECTORIES("${PROJECT_NAME}"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

GET_DIRECTORY_PROPERTY (glutess_has_root_project PARENT_DIRECTORY)

IF (NOT glutess_has_root_project)
    add_executable("${PROJECT_NAME}_test" "test/main.c" "test/common/tessellate.c")

    TARGET_LINK_LIBRARIES("${PROJECT_NAME}_test"
        glutess
    )
    TARGET_INCLUDE_DIRECTORIES("${PROJECT_NAME}_test"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/test/common"
    )

    FIND_PACKAGE(vkvg REQUIRED)
    FIND_PACKAGE(vkh REQUIRED)
    FIND_PACKAGE(Vulkan REQUIRED)
    FIND_PACKAGE(GLFW3 REQUIRED)

    FILE(GLOB vkvgsrc testvkvg/*.c)

    if (vkvg_FOUND)
        add_executable("${PROJECT_NAME}_vkvg" ${vkvgsrc} "test/common/tessellate.c")

        TARGET_INCLUDE_DIRECTORIES("${PROJECT_NAME}_vkvg" PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/test/common"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            ${Vulkan_INCLUDE_DIRS}
            ${vkvg_INCLUDE_DIRS}
            ${vkh_INCLUDE_DIRS}
            ${GLFW3_INCLUDE_DIR}
        )
        TARGET_LINK_LIBRARIES("${PROJECT_NAME}_vkvg"
            ${Vulkan_LIBRARIES}
            ${vkvg_LIBRARIES}
            ${vkh_LIBRARIES}
            ${GLFW3_LIBRARY}
            glutess
            "$<$<BOOL:${UNIX}>:-lm>"
        )
    ENDIF ()
ENDIF ()


