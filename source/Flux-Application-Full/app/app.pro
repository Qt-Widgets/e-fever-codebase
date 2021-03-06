TEMPLATE = app

TARGET = MYPROJECT

QT += qml quick
CONFIG += c++11

SOURCES += main.cpp

ROOTDIR = $$absolute_path($$PWD/..)

include(qpm.pri)
include($$ROOTDIR/cpp/cpp.pri)
include($$ROOTDIR/qml/qml.pri)
include($$ROOTDIR/deployment/deployment.pri)

DISTFILES +=     qpm.json

