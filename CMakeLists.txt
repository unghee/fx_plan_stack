cmake_minimum_required(VERSION 2.8.3)
project(fx_plan_stack)

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
	src/stack_util.cpp
)

if(COMPILE_SHARED)
	ADD_LIBRARY(fx_plan_stack SHARED ${FX_SRCFILES})
else()
	ADD_LIBRARY(fx_plan_stack STATIC ${FX_SRCFILES})
endif()


find_library(serialc_LIBRARY serialc PATHS ${PROJECT_SOURCE_DIR}/lib)
find_library(FlexSEA-Stack_LIBRARY FlexSEA-Stack-Plan PATHS ${PROJECT_SOURCE_DIR}/lib)

if ( serialc_LIBRARY )
         message("LIBRARY FOUND = " ${serialc_LIBRARY})
else()
        message("LIBRARY NOT FOUND: serialc")
endif()

if ( FlexSEA-Stack_LIBRARY )
         message("LIBRARY FOUND = " ${FlexSEA-Stack_LIBRARY})
else()
        message("LIBRARY NOT FOUND: Flexsea-Stack-Plan")
endif()

target_link_libraries(fx_plan_stack ${serialc_LIBRARY})
target_link_libraries(fx_plan_stack ${FlexSEA-Stack_LIBRARY})

IF(WIN32)
	target_link_libraries(fx_plan_stack setupapi)
ENDIF()

include_directories(
	include 
	libinclude 
	libinclude/flexseastack/flexsea-comm/inc 
	libinclude/flexseastack/flexsea-system/inc
	libinclude/flexseastack/flexsea-user/inc
)
