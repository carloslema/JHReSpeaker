#ifndef RESPEAKERMICARRAY_H
#define RESPEAKERMICARRAY_H

#include <libusb.h>
#include "../../hidapi/hidapi/hidapi.h"


class ReSpeakerMicArray
{
public:
    hid_device *handle ;
    ReSpeakerMicArray();

    // LED control
    int setLEDMode ( unsigned char mode, unsigned char data1, unsigned char data2, unsigned char data3 ) ;
    int setLEDAllOff ( void ) ;
    int setLEDAllRGB ( unsigned char redColor, unsigned char greenColor, unsigned char blueColor ) ;
    int setLEDListening ( unsigned char directionL, unsigned char directionH ) ;
    int setLEDWaiting ( void ) ;
    int setLEDSpeaking ( unsigned char strength, unsigned char directionL, unsigned char directionH ) ;
    int setLEDVolume ( unsigned char volume ) ;
    int setLEDData ( void ) ;
    int setLEDAutoVoiceLocated ( void ) ;
    // Write data of size len into register reg, return success
    int writeRegister(unsigned char reg, unsigned char * data, unsigned char len) ;
    // Read data of size len into ret at register reg, return success
    int readRegister(unsigned char reg, unsigned char * ret, unsigned char len) ;
    // Read Auto Report
    int readAutoReport  (unsigned short * angle, unsigned char *vadActivity) ;
    // Get the Voice Angle; returns -1 if not read
    int voiceAngle ( void ) ;
};

#endif // RESPEAKERMICARRAY_H
