#-------------------------------------------------
#
# Project created by QtCreator 2018-05-02T18:26:09
#
#-------------------------------------------------

QT       -= core gui

TARGET = fx_plan_stack
TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS \
			BOARD_TYPE_FLEXSEA_PLAN DEPHY

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH +=  inc         \
				include     \
				libinclude  \
				libinclude/flexseastack/flexsea-comm/inc \
				libinclude/flexseastack/flexsea-system/inc \

SOURCES +=                              \
		src/flexseaserial.cpp           \
		src/testserial.cpp              \
		src/flexseadevice.cpp           \
		src/commanager.cpp              \
		src/periodictask.cpp            \
		src/serialdriver.cpp            \
		src/flexseadeviceprovider.cpp   \
		src/commtester.cpp              \
		src/datalogger.cpp              \
		src/com_wrapper.cc              \
		src/comm_string_generation.cpp \
		src/revision.cpp

HEADERS += \
		include/flexseastack/flexseaserial.h    \
		include/flexseastack/testserial.h       \
		include/flexseastack/flexseadevicetypes.h \
		include/flexseastack/flexseadevice.h    \
		include/flexseastack/circular_buffer.h  \
		include/flexseastack/fxdata.h           \
		include/flexseastack/commanager.h       \
		include/flexseastack/periodictask.h     \
		include/flexseastack/flexsea-system/inc/flexsea_device_spec.h \
		include/flexseastack/flexsea-system/inc/flexsea_sys_def.h \
		include/flexseastack/serialdriver.h         \
		include/flexseastack/flexseadeviceprovider.h \
		include/flexseastack/commtester.h           \
		include/flexseastack/comm_string_generation.h \
		include/flexseastack/datalogger.h           \
		include/flexseastack/com_wrapper.h          \
		include/flexseastack/fx_device_defs.h       \
		include/flexseastack/rxhandler.h \
		include/revision.h

#flexsea stack headers
HEADERS += \
	libinclude/flexseastack/flexsea-shared/unity/unity.h \
	libinclude/flexseastack/flexsea-shared/unity/unity_internals.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_comm_def.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_buffers.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_circular_buffer.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_comm.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_payload.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_comm_multi.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_multi_frame_packet_def.h \
	libinclude/flexseastack/flexsea-comm/inc/flexsea_multi_circbuff.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_system.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_sys_def.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_global_structs.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_dataformats.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_data.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_external.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_sensors.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_calibration.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_control.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_tools.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_in_control.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_stream.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h \
	libinclude/flexseastack/flexsea-system/inc/flexsea_device_spec.h \
	libinclude/flexseastack/flexsea_board.h \
	libinclude/flexseastack/flexsea_config.h \
	libinclude/flexseastack/trapez.h \
	libinclude/flexseastack/flexsea-projects/inc/flexsea_cmd_user.h \
	libinclude/flexseastack/flexsea-dephy/CycleTester/inc/cmd-CycleTester.h \
	libinclude/flexseastack/flexsea-dephy/CycleTester/inc/user-mn-CycleTester.h

#serial lib headers
HEADERS += \
	libinclude/serial/serial.h \
	libinclude/serial/v8stdint.h \
	libinclude/serial/impl/unix.h \
	libinclude/serial/impl/win.h \

# serialc library for win32
win32: LIBS += -L$$PWD/lib/ -lserialc -lsetupapi -lFlexSEA-Stack-Plan

INCLUDEPATH += $$PWD/libinclude
DEPENDPATH += $$PWD/libinclude

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/lib/serialc.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/lib/libserialc.a

PRE_TARGETDEPS += $$PWD/lib/libFlexSEA-Stack-Plan.a

DESTDIR = $$PWD
