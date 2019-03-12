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
                                flexseastack/flexsea-comm/inc \
                                flexseastack/flexsea-system/inc \

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
		include/flexseastack/serialdriver.h         \
		include/flexseastack/flexseadeviceprovider.h \
		include/flexseastack/commtester.h           \
		include/flexseastack/comm_string_generation.h \
		include/flexseastack/datalogger.h           \
		include/flexseastack/com_wrapper.h          \
		include/flexseastack/fx_device_defs.h       \
		include/flexseastack/rxhandler.h \
		include/revision.h \
		include/git_rev_data.h

#flexsea stack headers
HEADERS += \
        flexseastack/flexsea-shared/unity/unity.h \
        flexseastack/flexsea-shared/unity/unity_internals.h \
        flexseastack/flexsea-comm/inc/flexsea.h \
        flexseastack/flexsea-comm/inc/flexsea_comm_def.h \
        flexseastack/flexsea-comm/inc/flexsea_buffers.h \
        flexseastack/flexsea-comm/inc/flexsea_circular_buffer.h \
        flexseastack/flexsea-comm/inc/flexsea_comm.h \
        flexseastack/flexsea-comm/inc/flexsea_payload.h \
        flexseastack/flexsea-comm/inc/flexsea_comm_multi.h \
        flexseastack/flexsea-comm/inc/flexsea_multi_frame_packet_def.h \
        flexseastack/flexsea-comm/inc/flexsea_multi_circbuff.h \
        flexseastack/flexsea-system/inc/flexsea_system.h \
        flexseastack/flexsea-system/inc/flexsea_sys_def.h \
        flexseastack/flexsea-system/inc/flexsea_global_structs.h \
        flexseastack/flexsea-system/inc/flexsea_dataformats.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_data.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_external.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_sensors.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_calibration.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_control.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_tools.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_in_control.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_stream.h \
        flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h \
        flexseastack/flexsea-system/inc/flexsea_device_spec.h \
        flexseastack/flexsea_board.h \
        flexseastack/flexsea_config.h \
        flexseastack/trapez.h \
        flexseastack/flexsea-projects/inc/flexsea_cmd_user.h \
        flexseastack/flexsea-dephy/CycleTester/inc/cmd-CycleTester.h \
        flexseastack/flexsea-dephy/CycleTester/inc/user-mn-CycleTester.h

#serial lib headers
HEADERS += \
        serial/serial.h \
        serial/v8stdint.h \
        serial/impl/unix.h \
        serial/impl/win.h \

# serialc library for win32
win32: LIBS += -L$$PWD/lib/ -lsetupapi

INCLUDEPATH += $$PWD/libinclude
DEPENDPATH += $$PWD/libinclude

# Update revision information
PRE_TARGETDEPS += git_rev_data.h
QMAKE_EXTRA_TARGETS += revisionTarget
revisionTarget.target = git_rev_data.h
win32: revisionTarget.commands = cd $$PWD && python.exe $$PWD/git-revision.py -o $$PWD/include/git_rev_data.h
else:  revisionTarget.commands = cd $$PWD; python.exe --version
revisionTarget.depends = FORCE

DESTDIR = $$PWD
