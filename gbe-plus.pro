# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

TEMPLATE = app
TARGET = gbe-plus
DESTDIR = ./Debug
CONFIG += debug
DEFINES += _WINDOWS GBE_QT_5 GBE_OGL
LIBS += -L"../../../../../../Qt/Qt5.12.12/5.12.12/msvc2017/lib" \
    -L"../../SDL2-2.0.18/lib/x86" \
    -lSDL2main \
    -lSDL2
DEPENDPATH += .
MOC_DIR += .
OBJECTS_DIR += debug
UI_DIR += .
RCC_DIR += .
include(gbe-plus.pri)
