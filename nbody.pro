TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += link_pkgconfig

PKGCONFIG += gl glfw3 glew epoxy glm

SOURCES += \
    nbody.cpp \
    main.cpp

HEADERS += \
    nbody.h \
    glview.h

DISTFILES += \
    glview.cpp.bak
