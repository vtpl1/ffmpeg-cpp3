#!/bin/bash

# Simple script to build some demos. No need to use more.
# don't forget to chmod it a +x 
# Copyright Eric Bachard / 2020 04 26
# This document is under GPL v3 license
# see : http://www.gnu.org/licenses/gpl-3.0.html
#

g++ -I../../sources  -Wall -D__linux__ -std=c++11  \
          -o build/remux_webcam remux_webcam.cpp \
          -L../../build/ -lffmpeg-cpp -lpthread \
          `pkg-config --cflags --libs libavutil libavcodec libavfilter libswscale libavdevice libavformat libswresample`

