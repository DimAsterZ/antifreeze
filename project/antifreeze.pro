#-------------------------------------------------
#
# Project created by QtCreator 2017-04-24T17:57:19
#
#-------------------------------------------------
QT -= core
QT -= gui

QMAKE_CXXFLAGS_GNUCXX11 = -std=c++17
QMAKE_CXXFLAGS_GNUCXX1Z = -std=c++17
CONFIG += c++17
win32: QMAKE_CXXFLAGS += /std:c++17
unix: QMAKE_CXXFLAGS += -std=c++17
unix: QMAKE_CXX = g++-9

TARGET = antifreeze
TEMPLATE = lib

DEFINES += ANTIFREEZE_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = $$PWD/../bin

################### include antifreeze 'include.pri' ###################
include($$PWD/../include.pri)
########################################################################

SOURCES += \
    ../source/AsyncOperProcessor.cpp \
    ../source/DispatchReactorStoppable.cpp \
    ../source/DeregisterableHandler.cpp \
    ../source/Reactor.cpp \
    ../source/EventHandler.cpp \
    ../source/MessageData.cpp \
    ../source/StoppingHandler.cpp

HEADERS += \
    ../include/Handle.hpp \
    ../include/AsyncOperProcessor.h \
    ../include/EventHandler.h \
    ../include/MessageData.h \
    ../include/Reactor.h \
    ../include/DispatchReactorStoppable.h \
    ../include/StoppingHandler.h \
    ../include/DeregisterableHandler.h \
    ../include/antifreeze_global.h

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}
