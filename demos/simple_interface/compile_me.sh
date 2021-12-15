#!/bin/bash

# Simple script to build the demo. No need to use more.
# don't forget to chmod it a +x 
# Copyright Eric Bachard / 2020 04 26
# This document is under GPL v2 license
# see : http://www.gnu.org/licenses/gpl-2.0.html
#
g++ -I..  -Wall -std=c++11  \
          -I../ffmpeg-cpp \
          -o simple_interface *.cpp \
          -L../../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libswscale libavformat libavfilter libswresample`
