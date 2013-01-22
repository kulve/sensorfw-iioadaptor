QT += dbus

CONFIG += qt debug warn_on link_prl link_pkgconfig plugin

PKGCONFIG += sensord
for(PKG, $$list($$unique(PKGCONFIG))) {
     !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
}

TEMPLATE = lib

TARGET       = iioadaptor

HEADERS += iioadaptor.h \
           iioadaptorplugin.h

SOURCES += iioadaptor.cpp \
           iioadaptorplugin.cpp
