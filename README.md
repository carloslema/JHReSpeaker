# JHReSpeaker
ReSpeaker for NVIDIA Jetson TX1 Development Kit

WARNING! This is a work in progress, the code is just an exploration of the ReSpeaker Far Field Mic Array (FFMA) firmware at this point

ReSpeaker Example contains an early, not even close to ready for prime time GUI to control the ReSpeaker LEDs, and do basic sound recording.

Intall the prerequisites:

$ ./installPrerequisites.sh

These will install the needed development libraries, along with Qt Creator and Qt multimedia. Qt is used for the ReSpeakerExample. If you are only interested in the ReSpeaker library, please modify the shell file accordingly.

Some Notes:

1) There is a major issue with fwupd in Ubuntu for USB audio devices. The ReSpeaker FFMA frequently will not work if the fwupd service is running. To kill the service:

$ sudo killall fwupd

2) Make sure that USB autosuspend is turned off on the Jetson:

$ sudo bash -c ‘echo -1 > /sys/module/usbcore/parameters/autosuspend’






