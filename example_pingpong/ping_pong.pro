TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS_GNUCXX11 = -std=c++17
QMAKE_CXXFLAGS_GNUCXX1Z = -std=c++17
win32: QMAKE_CXXFLAGS += /std:c++17
unix: QMAKE_CXXFLAGS += -std=c++17
unix: QMAKE_CXX = g++-9

DESTDIR = $$PWD/../binexample
TARGET = ping_pong

SOURCES += \
        main.cpp

################# include antifreeze #########################
include($$PWD/../../antifreeze/include.pri)
unix:LIBS += -L$$PWD/../../antifreeze/bin
##############################################################

unix:LIBS += -lpthread

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

HEADERS += \
    helper.h
