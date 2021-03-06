# Copyright (c) <2014-2017> <Newton Game Dynamics>
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely.

cmake_minimum_required(VERSION 3.10.0)

set (projectName "dgNewtonAvx2")
message (${projectName})

# low level core
file(GLOB source *.cpp *.h)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:fast")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /arch:AVX2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")

add_definitions(-DNEWTONCPU_EXPORTS)
add_library(${projectName} SHARED ${source})
target_link_libraries (${projectName} dTimeTracker)

if (MSVC)
	set_target_properties(${projectName} PROPERTIES COMPILE_FLAGS "/YudgNewtonPluginStdafx.h")
	set_source_files_properties(dgNewtonPluginStdafx.cpp PROPERTIES COMPILE_FLAGS "/YcdgNewtonPluginStdafx.h")
endif(MSVC)


#install(TARGETS ${projectName} RUNTIME DESTINATION ${dllPath})
#install(TARGETS ${projectName} ARCHIVE DESTINATION ${libraryPath})

add_custom_command(
	TARGET ${projectName} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	ARGS -E copy $(TargetPath) ${CMAKE_INSTALL_BINDIR}/newtonPlugins/${CMAKE_CFG_INTDIR}/$(TargetFileName))


if (BUILD_SANDBOX_DEMOS)
	add_custom_command(
		TARGET ${projectName} POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy $(TargetPath) ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/newtonPlugins/${CMAKE_CFG_INTDIR}/$(TargetFileName))
endif ()
