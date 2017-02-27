#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "respeakermicarray.h"

ReSpeakerMicArray::ReSpeakerMicArray()
{
    handle = hid_open(0x2886, 0x07, NULL);
    if (handle == 0) {
        std::cout << "No USB Handle" << std::endl   ;
    }
}

//
// Control the Microphone Array LEDs
//
int ReSpeakerMicArray::setLEDMode ( unsigned char mode, unsigned char data1, unsigned char data2, unsigned char data3 ) {
    // unsigned char buf[9] ;
    unsigned char buf[9] = { 0x0, 0x0, 0x0, 0x4, 0x0, mode, data1, data2, data3 } ;
    int res ;
    res = hid_write(handle,buf,9) ;
    return res ;
}


// Turns all of the perimeter lights off
int ReSpeakerMicArray::setLEDAllOff ( void ) {
    return setLEDMode(0,0,0,0) ;
}

int ReSpeakerMicArray::setLEDAllRGB ( unsigned char redColor, unsigned char greenColor, unsigned char blueColor ) {
    return setLEDMode(1,blueColor,greenColor,redColor) ;
}

int ReSpeakerMicArray::setLEDListening ( unsigned char directionL, unsigned char directionH ) {
    return setLEDMode(2,0,directionL, directionH) ;
}

// Performs race around the perimeter
int ReSpeakerMicArray::setLEDWaiting ( void ) {
    return setLEDMode(3,0,0,0) ;
}

// Performs race around the perimeter
int ReSpeakerMicArray::setLEDSpeaking ( unsigned char strength, unsigned char directionL, unsigned char directionH ) {
    return setLEDMode(4,strength,directionL, directionH) ;
}

int ReSpeakerMicArray::setLEDVolume ( unsigned char volume ) {
    return setLEDMode(5,0,0,volume) ;
}

int ReSpeakerMicArray::setLEDData ( void ) {
    return setLEDMode(6,0,0,0) ;
}

// This appears to set the device lights back to "looking for voices" modes
int ReSpeakerMicArray::setLEDAutoVoiceLocated ( void ) {
    return setLEDMode(7,0,0,0) ;
}

// Write data of size len into register reg, return success
int ReSpeakerMicArray::writeRegister(unsigned char reg, unsigned char * data, unsigned char len) {
    unsigned char buf[len + 5];
    int res;
    // Set a register on the mic --
    buf[0] = 0; // First byte is report number
    buf[1] = reg; // register #
    buf[2] = 0; // space
    buf[3] = len; // length
    buf[4] = 0; // space
    // for(int i=0i;i<len;i++) buf[5+i] = data[i];
    for(int i=0;i<len;i++) {
        buf[5+i] = data[i];
    }
    res = hid_write(handle, buf, len+5);
    return res;
}

// Read data of size len into ret at register reg, return success
int ReSpeakerMicArray::readRegister(unsigned char reg, unsigned char * ret, unsigned char len) {
    int res;
    unsigned char buf[9];
    buf[0] = 0;
    buf[1] = reg;
    buf[2] = 0x80;
    buf[3] = len;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    // To read a register, send register with 0x80, and then read it back.
    // If blocking is off, the read will return none if it's too soon after
    res = hid_write(handle, buf, 7);

    res = hid_read(handle, buf, 7+len);
    if (res == 0) {
      printf("Too soon after write in read register\n");
    }
    if(buf[0] == reg) {
        for(int i=0;i<len;i++) {
            ret[i] = buf[4+i];
        }
        return 1;
    }
    return 0;

}

// Returns 1 if auto report was read, 0 otherwise
int ReSpeakerMicArray::readAutoReport  (unsigned short * angle, unsigned char *vadActivity) {
    int res;
    unsigned char buf[9];
    angle[0] = 0;
    vadActivity[0] = 0;

    hid_set_nonblocking(handle, 1);
    res = hid_read(handle, buf, 9);
    hid_set_nonblocking(handle,0) ;
    if (res > 4) {
        if(buf[0] == 0xFF) {
            int soundAngle = buf[6]*256 + buf[5];
            if (soundAngle != 0) {
                printf("soundAngle: %d\n",soundAngle) ;
                fflush(stdout);
            }
            angle[0] = buf[6]*256 + buf[5];
            vadActivity[0] = buf[4];
            return 1;
        }
    }
    return 0;
}

// Return the voice angle
// Returns -1 is unable
int ReSpeakerMicArray::voiceAngle() {
    unsigned char buffer[2] ;
    int res ;
    res = readRegister(0x44, buffer, 2);
    if (res) {
       printf("Buffer[0]: %d Buffer[1]: %d\n",buffer[0],buffer[1]) ;
       fflush(stdout);
    }
    return -1 ;
}
