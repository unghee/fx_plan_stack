cmake_minimum_required(VERSION 2.8.3)
project(fx_plan_stack)

if(WIN32)
	set(PLAT_DIR "win")
elseif(UNIX AND NOT APPLE)
	set(PLAT_DIR "unix")
else()
	message(FATAL_ERROR " host OS not supported, exiting now!")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(PLAT_DIR "${PLAT_DIR}64")
else()
	set(PLAT_DIR "${PLAT_DIR}32")
endif()

message(STATUS "PLAT_DIR is set to ${PLAT_DIR}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/plan_definitions.cmake)

option(COMPILE_SHARED "compile as a shared lib vs. a static lib" ON)

include_directories(
	include
	include/flexseastack
	include/flex_lib
	unity
	${CMAKE_CURRENT_SOURCE_DIR}/serial/include
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-comm/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-dephy/CycleTester/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-dephy/DpEb42/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-dephy/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-shared/unity
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-shared/i2t/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-shared/utilities/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-shared/logger/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-system/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-projects/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-projects/ActPack/inc
	${CMAKE_CURRENT_SOURCE_DIR}/flexsea-projects/Rigid/inc
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/flexsea-comm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/flexsea-system)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/flexsea-projects)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/flexsea-dephy)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/flexsea-shared)
add_subdirectory(serial)

file(GLOB PLAN_STACK_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/flex_lib/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/unity/*.c
)

set(FX_SRCFILES
	${PLAN_STACK_SRCS}
	${FLEX_COMM_SRCS}
	${FLEX_DEPHY_SRCS}
	${FLEX_PROJECTS_SRCS}
	${FLEX_SYSTEM_SRCS}
	${FLEX_SHARED_SRCS}
	${SERIAL_SRCS}
)

add_library(fx_plan_objs OBJECT ${FX_SRCFILES})

add_library(fx_plan_stack SHARED $<TARGET_OBJECTS:fx_plan_objs>)
add_library(fx_plan_stack_static STATIC $<TARGET_OBJECTS:fx_plan_objs>)

if(WIN32)
	message("Win32 detected")
	target_link_libraries(fx_plan_stack setupapi)
	set_target_properties(fx_plan_stack_static PROPERTIES LINK_FLAGS "/WHOLEARCHIVE")
	target_link_libraries(fx_plan_stack_static setupapi)
endif()
target_link_libraries(fx_plan_stack pthread)
target_link_libraries(fx_plan_stack_static pthread)


## Force generation of the revision header file BEFORE building targets
##  Create a psuedo-target to be created before the library
if(WIN32)
add_custom_target(
	revisionTarget
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update_revs.bat ${CMAKE_CURRENT_SOURCE_DIR}/include/git_rev_data.h
	COMMENT "Updating revision information"
)

else()
add_custom_target(
   revisionTarget
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update_revs.sh ${CMAKE_CURRENT_SOURCE_DIR}/git-revision.py ${CMAKE_CURRENT_SOURCE_DIR}/include/git_rev_data.h
   COMMENT "Updating revision information"
)

endif()

#Now add the psuedo-target to the dependency list for the project
add_dependencies(fx_plan_objs revisionTarget)
#add_dependencies(fx_plan_stack_static revisionTarget)

# output folder naming stuff
set(OUT_PREF "libs")

set_target_properties( fx_plan_stack 
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUT_PREF}"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUT_PREF}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUT_PREF}"
)

set_target_properties( fx_plan_stack_static
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUT_PREF}"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUT_PREF}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUT_PREF}"
)
