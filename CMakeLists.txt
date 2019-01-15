cmake_minimum_required(VERSION 2.8.3)
project(fx_plan_stack)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

ADD_LIBRARY(fx_plan_objs OBJECT ${FX_SRCFILES})


ADD_LIBRARY(fx_plan_stack SHARED $<TARGET_OBJECTS:fx_plan_objs>)
ADD_LIBRARY(fx_plan_stack_static STATIC $<TARGET_OBJECTS:fx_plan_objs>)



find_library(serialc_LIBRARY serialc PATHS ${PROJECT_SOURCE_DIR}/lib)
find_library(FlexSEA-Stack_LIBRARY FlexSEA-Stack-Plan PATHS ${PROJECT_SOURCE_DIR}/lib)

target_link_libraries(fx_plan_stack ${serialc_LIBRARY})
target_link_libraries(fx_plan_stack ${FlexSEA-Stack_LIBRARY})

target_link_libraries(fx_plan_stack_static ${serialc_LIBRARY})
target_link_libraries(fx_plan_stack_static ${FlexSEA-Stack_LIBRARY})

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

# output folder naming stuff

if(WIN32)
	set(OUT_PREF "win")
else()
	set(OUT_PREF "unix")	
	set (CMAKE_CXX_FLAGS "-fPIC")
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
