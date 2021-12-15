#!/bin/bash

# Simple script to build some demos. No need to use more.
# don't forget to chmod it a +x 
# Copyright Eric Bachard / 2020 04 26
# This document is under GPL v3 license
# see : http://www.gnu.org/licenses/gpl-3.0.html
#

g++ -I../../sources  -Wall -D__linux__ -std=c++11  \
          -o build/webcam_stream_to_MKV webcam_stream_to_MKV.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libavfilter libswscale libavdevice libavformat libswresample`

g++ -I../../sources  -Wall -D__linux__ -std=c++11  \
          -o build/webcam_stream_to_MPEG2 webcam_stream_to_MPEG2.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libavfilter libswscale libavdevice libavformat libswresample`

g++ -I../../sources  -Wall -D__linux__ -std=c++11  \
          -o build/webcam_stream_to_MPEG4 webcam_stream_to_MPEG4.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libavfilter libswscale libavdevice libavformat libswresample`

g++ -I../../sources  -Wall -D__linux__ -std=c++11  \
          -o build/webcam_stream_to_VP9 webcam_stream_to_VP9.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libavfilter libswscale libavdevice libavformat libswresample`


g++ -I../../sources  -Wall -D__linux__ -std=c++11  \
          -o build/webcam_stream_to_SOUND webcam_stream_to_SOUND.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libavfilter libswscale libavdevice libavformat libswresample`

