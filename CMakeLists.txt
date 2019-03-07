cmake_minimum_required(VERSION 2.8.3)
project(fx_plan_stack)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(COMPILE_SHARED "compile as a shared lib vs. a static lib" ON)

set(FX_SRCFILES
	src/flexseaserial.cpp
	src/testserial.cpp
	src/flexseadevice.cpp
	src/commanager.cpp
	src/periodictask.cpp
	src/serialdriver.cpp
	src/flexseadeviceprovider.cpp
	src/commtester.cpp
	src/datalogger.cpp
	src/com_wrapper.cc
	src/comm_string_generation.cpp
	src/revision.cpp
)

ADD_LIBRARY(fx_plan_objs OBJECT ${FX_SRCFILES})


ADD_LIBRARY(fx_plan_stack SHARED $<TARGET_OBJECTS:fx_plan_objs>)
ADD_LIBRARY(fx_plan_stack_static STATIC $<TARGET_OBJECTS:fx_plan_objs>)

if(CMAKE_SIZEOF_VOID_P GREATER 4)
	if(WIN32)
	find_library(serialc_LIBRARY serialc PATHS ${PROJECT_SOURCE_DIR}/lib64)
	find_library(FlexSEA-Stack_LIBRARY FlexSEA-Stack-Plan PATHS ${PROJECT_SOURCE_DIR}/lib64)
	else()
	find_library(serialc_LIBRARY serialc PATHS ${PROJECT_SOURCE_DIR}/unix64)
	find_library(FlexSEA-Stack_LIBRARY FlexSEA-Stack-Plan PATHS ${PROJECT_SOURCE_DIR}/unix64)
	endif()
else()
find_library(serialc_LIBRARY serialc PATHS ${PROJECT_SOURCE_DIR}/lib)
find_library(FlexSEA-Stack_LIBRARY FlexSEA-Stack-Plan PATHS ${PROJECT_SOURCE_DIR}/lib)
endif()

# --whole-archive
target_link_libraries(fx_plan_stack -Wl,--whole-archive ${serialc_LIBRARY} -Wl,--no-whole-archive)
target_link_libraries(fx_plan_stack -Wl,--whole-archive ${FlexSEA-Stack_LIBRARY} -Wl,--no-whole-archive)

target_link_libraries(fx_plan_stack_static -Wl,--whole-archive ${serialc_LIBRARY} -Wl,--no-whole-archive)
target_link_libraries(fx_plan_stack_static -Wl,--whole-archive ${FlexSEA-Stack_LIBRARY} -Wl,--no-whole-archive)

IF(WIN32)
	target_link_libraries(fx_plan_stack setupapi)
	target_link_libraries(fx_plan_stack_static setupapi)
ENDIF()

include_directories(
	include 
	libinclude 
	libinclude/flexseastack/flexsea-comm/inc 
	libinclude/flexseastack/flexsea-system/inc
	libinclude/flexseastack/flexsea-user/inc
)

## Force generation of the revision header file BEFORE building targets
##  Create a psuedo-target to be created before the library
IF(WIN32)
add_custom_target(
    revisionTarget
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update_revs.bat ${CMAKE_CURRENT_SOURCE_DIR}/include/git_rev_data.h
    COMMENT "Updating revision information"
)

ELSE()
add_custom_target(
   revisionTarget
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update_revs.sh ${CMAKE_CURRENT_SOURCE_DIR}/git-revision.py ${CMAKE_CURRENT_SOURCE_DIR}/include/git_rev_data.h
   COMMENT "Updating revision information"
)

ENDIF()

#Now add the psuedo-target to the dependency list for the project
add_dependencies(fx_plan_objs revisionTarget)
#add_dependencies(fx_plan_stack_static revisionTarget)

# output folder naming stuff

if(WIN32)
	set(OUT_PREF "win")
else()
	set(OUT_PREF "unix")	
	target_link_libraries(fx_plan_stack pthread)
	target_link_libraries(fx_plan_stack_static pthread)
endif()

if(CMAKE_SIZEOF_VOID_P GREATER 4)
	set(OUT_PREF "${OUT_PREF}64")
endif()

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
