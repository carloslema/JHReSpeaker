#-------------------------------------------------
#
# Project created by QtCreator 2017-02-11T15:50:39
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ReSpeakerExample
TEMPLATE = subdirs

# Ensure that library is built before application
CONFIG  += ordered

SUBDIRS += 3rdparty/fftreal/fftreal.pro

SUBDIRS += src


FORMS    += mainwindow.ui



# unix:!macx: LIBS += -L$$PWD/../../../../usr/local/lib/ -lhidapi-libusb
LIBS += /usr/lib/aarch64-linux-gnu/libusb-1.0.so
LIBS += /usr/lib/aarch64-linux-gnu/libpthread.so

INCLUDEPATH += $$PWD/../../../../usr/local/include/
INCLUDEPATH += /usr/include/libusb-1.0/
DEPENDPATH += $$PWD/../../../../usr/local/include

# unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/local/lib/libhidapi-libusb.a


DISTFILES +=


