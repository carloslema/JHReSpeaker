include(micarray.pri)

TEMPLATE = app

TARGET = micArray

QT       += multimedia widgets

SOURCES += main.cpp\
    mainwindow.cpp \
    audiointerface.cpp \
    wavfile.cpp \
    utils.cpp \
    tonegenerator.cpp \
    spectrumanalyser.cpp \
    frequencyspectrum.cpp \
    ../../src/respeakermicarray.cpp \
    progressbar.cpp \
    levelmeter.cpp \
    spectrograph.cpp \
    waveform.cpp \
    ../../hidapi/libusb/hid.c

HEADERS  += mainwindow.h \
    audiointerface.h \
    wavfile.h \
    utils.h \
    micarray.h \
    tonegenerator.h \
    spectrumanalyser.h \
    frequencyspectrum.h \
    ../../src/respeakermicarray.h \
    levelmeter.h \
    progressbar.h \
    spectrograph.h \
    waveform.h \
    ../../hidapi/hidapi/hidapi.h

FORMS    += ../mainwindow.ui



# unix:!macx: LIBS += -L$$PWD/../../../../usr/local/lib/ -lhidapi-libusb
LIBS += /usr/lib/aarch64-linux-gnu/libusb-1.0.so
LIBS += /usr/lib/aarch64-linux-gnu/libpthread.so

INCLUDEPATH += $$PWD/../../../../usr/local/include/
INCLUDEPATH += /usr/include/libusb-1.0/
DEPENDPATH += $$PWD/../../../../usr/local/include

# unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/local/lib/libhidapi-libusb.a


fftreal_dir = ../3rdparty/fftreal

INCLUDEPATH += $${fftreal_dir}

hidapi_dir = ../../hidapi/hidapi/
INCLUDEPATH += $${hidapi_dir}

# RESOURCES = micarray.qrc

# Dynamic linkage against FFTReal DLL
!contains(DEFINES, DISABLE_FFT) {
    macx {
        # Link to fftreal framework
        LIBS += -F$${fftreal_dir}
        LIBS += -framework fftreal
    } else {
        LIBS += -L..$${spectrum_build_dir}
        LIBS += -lfftreal
    }
}

RESOURCES += \
    micarray.qrc


