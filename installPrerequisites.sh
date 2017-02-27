#!/bin/bash
# NVIDIA Jetson Development Kit
# Install Prerequisites for ReSpeaker Far Field Microphone Array library

sudo apt-get install libudev-dev libusb-1.0-0-dev -y
git clone https://github.com/signal11/hidapi.git
cd hidapi
git checkout a6a622ffb680c55da0de787ff93b80280498330f
cd ..
sudo apt-get install qt5-default qtcreator -y 
# Add Multimedia extensions for easier audio access
sudo apt-get install qtmultimedia5-dev qtmultimedia5-examples -y


# Setup the permissions for the mic array
sudo cp rules/90-respeaker-mic-lisbusb.rules /etc/udev/rules.d
sudo udevadm control --reload-rules

