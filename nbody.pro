TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += link_pkgconfig

PKGCONFIG += gl glfw3 glew glm

SOURCES += \
    nbody.cpp \
    main.cpp \
    glview.cpp

HEADERS += \
    nbody.h \
    glview.h
