cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( VideodrommVisualizer )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Cinder" ABSOLUTE )
get_filename_component( PROG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )
get_filename_component( BLOCKS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Cinder/blocks" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

set( SRC_FILES
	${PROG_DIR}/src/VideodrommVisualizerApp.cpp
	${BLOCKS_DIR}/Cinder-ImGui/src/CinderImGui.cpp
	${BLOCKS_DIR}/Cinder-ImGui/lib/imgui/imgui.cpp
	${BLOCKS_DIR}/Cinder-ImGui/lib/imgui/imgui_demo.cpp
	${BLOCKS_DIR}/Cinder-ImGui/lib/imgui/imgui_draw.cpp
	${BLOCKS_DIR}/Cinder-Warping/src/Warp.cpp
	${BLOCKS_DIR}/Cinder-Warping/src/WarpBilinear.cpp
	${BLOCKS_DIR}/Cinder-Warping/src/WarpPerspective.cpp
	${BLOCKS_DIR}/Cinder-Warping/src/WarpPerspectiveBilinear.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/LiveParam.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDAnimation.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDConsole.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDFbo.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDLog.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDMix.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDRouter.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDSession.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDSettings.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDShader.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDTexture.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUI.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIAnimation.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIAudio.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIBlend.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIChannels.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIColor.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIFbos.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIMidi.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIMouse.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIOsc.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIShaders.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUITempo.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUITextures.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUIWebsockets.cpp
	${BLOCKS_DIR}/Cinder-Videodromm/src/VDUtils.cpp
	${BLOCKS_DIR}/OSC/srv/cinder/osc/Osc.cpp
)
set(PROJECT_INCLUDE_FILES
    ${PROG_DIR}/include
	${BLOCKS_DIR}/Cinder-ImGui/include
	${BLOCKS_DIR}/Cinder-ImGui/lib/imgui
	${BLOCKS_DIR}/Cinder-Warping/include
	${BLOCKS_DIR}/Cinder-Videodromm/include
	${BLOCKS_DIR}/OSC/srv/cinder/osc
)
ci_make_app(
	SOURCES     ${SRC_FILES}
	INCLUDES ${PROJECT_INCLUDE_FILES}
	CINDER_PATH ${CINDER_PATH}
	BLOCKS OSC
)
