Eric Bachard, 2020/06/02


This is experimental. Please do not use: you can damage your machine, and maybe lose your data.


- LAST : remux_webcam allows to record audio and video in sync.

N.B. : there is a dedicated shell script to build remux_webcam

- works with mjpeg webcams like Logitech webcams (C920, C922, Brio stream models, other are untested)
- don't forget /dev/video0 is used (adapt to your devices if you need to select another webcam)
- all binaries should produce vidéos, that can be read by mpv, ffplay or any video reader (ffprobe returns ok)
- containers: .mp4 is ok, .mkv is close to be ok, .avi is ok, and should work, but not choosen
- audio codec : aac is ok
- hevc is untested (TODO)
- video codec : h264 is ok
- video codec : VP9 is ok
- the machine load is important, precisely for h264 encoding. Due to hardware implementation (e.g. VAAPI) not ready.
- MPEG4 : seems to work (WIP)
- MPEG2 : seems to work (WIP)
- MKV + sound : does not work (sound = half video time)

To build : 

chmod ug+x compile_me.sh
./compile_me.sh



If nothing's wrong (e.g. you built ffmpeg libraries with --enable-libmp3-lame --enable-libx264 --enable-gpl,
AND you installed everything, it should work.

If something goes wrong, please file an issue !

Known issues :

- no sound yet, but should work 
- filters are untested
- timings are wrong (probably due to fantaisist use of frameRate that I need to learn)

