#!/bin/bash

# Simple script to build the demo. No need to use more.
# don't forget to chmod it a +x 
# Copyright Eric Bachard / 2020 04 26
# This document is under GPL v2 license
# see : http://www.gnu.org/licenses/gpl-2.0.html
#


#DEBG="-g -DDEBUG"

#BUIlD_DIR="./build"

g++ -g ${DEBG} -I.. -Wall -std=c++11  \
          -I../../sources \
          -o encode_video_to_MPEG2 encode_to_MPEG2_video.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libswscale libavformat libavdevice`


g++ ${DEBG} -I.. -Wall -std=c++11  \
          -I../../sources \
          -o encode_video_to_MPEG4 encode_to_MPEG4_video.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libswscale libavformat libavdevice`


g++ ${DEBG} -I.. -Wall -std=c++11  \
          -I../../sources \
          -o encode_video_to_H264 encode_to_H264_video.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libswscale libavformat libavdevice`

g++ ${DEBG} -I.. -Wall -std=c++11  \
          -I../../sources \
          -o encode_MJPEG_webcam_to_VP9_video encode_MJPEG_webcam_to_VP9_video.cpp \
          -L../../build/ -lffmpeg-cpp \
          `pkg-config --cflags --libs libavutil libavcodec libswscale libavformat libavdevice`

