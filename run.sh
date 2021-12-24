#!/bin/bash
/workspaces/ffmpeg-cpp3/build/entrypoint/ffmpeg_cpp3_tester -i rtsp://admin:AdmiN1234@192.168.0.58/h264/ch1/main/ -o rtmp://192.168.1.116:9101
.\\build\\entrypoint\\Debug\\ffmpeg_cpp3_tester.exe /i rtsp://admin:AdmiN1234@192.168.0.58/h264/ch1/main/ /o rtmp://192.168.1.116:9101
.\\build\\entrypoint\\Release\\media_converter.exe -i rtsp://admin:AdmiN1234@192.168.0.58/h264/ch1/main/ -o rtmp://192.168.1.116:9101