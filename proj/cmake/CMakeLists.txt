cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( opengl-PostProcessingAA )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Cinder" ABSOLUTE )
get_filename_component( PROG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

set( SRC_FILES
	${PROG_DIR}/src/VideodrommVisualizerApp.cpp
)
set(PROJECT_INCLUDE_FILES
    ${PROG_DIR}/include
)
ci_make_app(
	SOURCES     ${SRC_FILES}
	INCLUDES ${PROJECT_INCLUDE_FILES}
	CINDER_PATH ${CINDER_PATH}
)
