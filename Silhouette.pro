QT += gui core

CONFIG += c++11

TARGET = Silhouette
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    Silhouette.cpp \
    vbomeshadj.cpp

HEADERS += \
    Silhouette.h \
    vbomeshadj.h

OTHER_FILES += \
    vshader.txt \
    fshader.txt \
    gshader.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader.txt \
    vshader.txt \
    gshader.txt
