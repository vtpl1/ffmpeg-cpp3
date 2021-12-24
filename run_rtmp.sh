#!/bin/bash
/workspaces/ffmpeg-cpp3/build/entrypoint/ffmpeg_cpp3_tester -i rtmp://0.0.0.0:9101 -o ./videos/play.m3u8
.\\build\\entrypoint\\Debug\\ffmpeg_cpp3_tester.exe /i rtmp://0.0.0.0:9101 /o .\\videos\\play.m3u8
.\\build\\entrypoint\\Release\\media_converter.exe -i rtmp://0.0.0.0:9101 -o play.m3u8
.\\build\\entrypoint\\Release\\media_converter.exe -i rtmp://0.0.0.0:9101 -o .\\videos\\play.m3u8