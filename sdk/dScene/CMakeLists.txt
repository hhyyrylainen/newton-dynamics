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

set (projectName "dScene")
message (${projectName})

# low level core
file(GLOB CPP_SOURCE *.cpp)
file(GLOB HEADERS *.h)

if(NEWTON_BUILD_SHARED_LIBS)
    if(MSVC)
        add_definitions(-D_DSCENE_DLL)
        add_definitions(-D_DSCENE_EXPORTS)
        add_definitions(-D_DCONTAINERS_DLL)
    endif(MSVC)
    add_library(${projectName} SHARED ${CPP_SOURCE})
else(NEWTON_BUILD_SHARED_LIBS)
    if (MSVC)
        add_definitions(-D_NEWTON_STATIC_LIB)
        add_definitions(-D_CNEWTON_STATIC_LIB)
        add_definitions(-D_CUSTOM_JOINTS_STATIC_LIB)
    endif(MSVC)

    add_library(${projectName} STATIC ${CPP_SOURCE})
endif(NEWTON_BUILD_SHARED_LIBS)

target_link_libraries (${projectName} Newton dContainers dMath dCustomJoints tinyxml)
if (BUILD_PROFILER)
    target_link_libraries (${projectName} dTimeTracker)
endif()

if (MSVC)
	set_target_properties(${projectName} PROPERTIES COMPILE_FLAGS "/YudSceneStdafx.h")
	set_source_files_properties(dSceneStdafx.cpp PROPERTIES COMPILE_FLAGS "/YcdSceneStdafx.h")
	if (BUILD_SANDBOX_DEMOS)
		add_custom_command(
			TARGET ${projectName} POST_BUILD
			COMMAND ${CMAKE_COMMAND}
			ARGS -E copy $(TargetPath) ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/$(TargetFileName))
	endif ()
endif(MSVC)

install(TARGETS ${projectName}
       LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
       ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${projectName})
