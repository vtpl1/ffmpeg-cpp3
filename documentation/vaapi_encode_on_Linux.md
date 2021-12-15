Source : https://gist.github.com/Brainiarc7/95c9338a737aa36d9bb2931bed379219

DISCLAIMER :  BELOW IS A COPY PASTE OF THE PREVIOUS LINK


Using VAAPI's hardware accelerated video encoding on Linux with Intel's hardware on FFmpeg and libav
------------------------------------------------------------------------

Hello, brethren :-)

As it turns out, the current version of FFmpeg (version 3.1 released
earlier today) and libav (master branch) supports full H.264 and HEVC encode in VAAPI on
supported hardware that works reliably well to be termed
"production-ready".

Assumptions:
------------

Before taking on this manual, the author assumes that:

1. The end-user can comfortably install and configure their Linux distribution of choice.
2.  The end user can install, upgrade, downgrade and resolve both conflicts and dependency resolution of packages on his/her distribution's package manager.
3. That the user is comfortable with the Linux terminal, and can navigate through it.
4. Basic competence on the shell, such as reading man files, using a text editor of choice, manipulating file operations on the same, etc is assumed.

And as an indemnity clause, I, the author, will **not be liable** for any damage, implied or otherwise, to your files, hardware or the stability of your machine as a consequence to using these instructions to achieve a similar feat as described in this gist.

Implications:
-------------

It means that when you're encoding content for use with your blogs or some fancy youtube download, you can do it much, much faster on hardware with **lower processor utilization** (so you can multi-task) , **lesser heat output** and, as a plus, is **significantly faster** (As tested on my end, ~8.7x for 1080p and ~4.2x for 4k encodes with reference media) compared to a pure, software-based approach as offered by libx264 and similar implementations, albeit at an **acceptable quality compromise**.

Here goes:

First, you will need to build ffmpeg (and libav,as per your preferences) with appropriate arguments.
 *--enable-vaapi* switch should be enough, though.

Here are my build options (Note that I load ffmpeg and libav via the module system):

FFmpeg's module files are [here](https://github.com/Brainiarc7/personal-environment-modulefiles/tree/master/modulefiles/ffmpeg), and as more versions are compiled, more modules will be added.
Libav's module files are [here](https://github.com/Brainiarc7/personal-environment-modulefiles/tree/master/modulefiles/libav), and as more versions are compiled, more modules will be added.

**FFmpeg**'s configuration switches used:

    ./configure --enable-nonfree --enable-gpl --enable-version3
    --enable-libass --enable-libbluray --enable-libmp3lame
    --enable-libopencv --enable-libopenjpeg --enable-libopus
    --enable-libfaac --enable-libfdk-aac --enable-libtheora
    --enable-libvpx --enable-libwebp --enable-opencl --enable-x11grab
    --enable-opengl --cpu=native --enable-nvenc --enable-vaapi
    --enable-vdpau  --enable-ladspa --enable-libass  --enable-libgsm
    --enable-libschroedinger --enable-libsmbclient --enable-libsoxr
    --enable-libspeex --enable-libssh --enable-libwavpack --enable-libxvid
    --enable-libx264 --enable-libx265 --enable-netcdf  --enable-openal
    --enable-openssl --enable-cuda --prefix=/apps/ffmpeg/git --enable-omx


**Libav**'s configuration switches used:

    ./configure --prefix=/apps/libav/11.7 --enable-gpl --enable-version3
    --enable-nonfree --enable-runtime-cpudetect --enable-gray
    --enable-vaapi --enable-vdpau --enable-vda --enable-libmp3lame
    --enable-libopenjpeg --enable-libopus --enable-libfaac
    --enable-libfdk-aac --enable-libtheora --enable-libvpx
    --enable-libwebp  --enable-x11grab  --cpu=native  --enable-vaapi
    --enable-vdpau  --enable-libgsm --enable-libschroedinger
    --enable-libspeex --enable-libwavpack --enable-libxvid
    --enable-libx264 --enable-libx265 --enable-openssl --enable-nvenc
    --enable-cuda --enable-omx

Then run *make* and `make install` to build and install the toolkits respectively.

**Warning**: These options are for reference only, a useful FFmpeg build will require you to install appropriate dependencies for some build options as suited to your environment and platform. **Modify as needed.** Also see the indemnity clause at the top of this document.

Here are the dependencies I had to install on my end (without acounting for the OpenMAX IL bellagio back-end):

    sudo apt-get install yasm ladspa-sdk ladspa-foo-plugins ladspalist libass5 libass-dev libbluray-bdj libbluray-bin libbluray-dev libbluray-doc libbluray1 libmp3lame-dev \ libmp3lame-ocaml libmp3lame-ocaml-dev libmp3lame0 libsox-fmt-mp3 libopencv-* opencv-* python-cv-bridge python-image-geometry python-opencv python-opencv-apps gstreamer1.0-vaapi gstreamer1.0-vaapi-doc libopenjp2-* libopenjp2-7-dev libopenjp2-7-dbg libopenjp3d7 libopenjpeg-dev libopenjpeg-java libopenjpeg5 libopenjpeg5-dbg libopenjpip7 openjpeg-tools libopus-dbg libopus-dev libopus-doc libopus0 libtag1-dev libtag1-doc libtag1v5 libtagc0 libtagc0-dev libopus-ocaml libopus-ocaml-dev libopusfile-dev libopusfile-doc libopusfile0 libvorbis-java opus-tools opus-tools-dbg libfaac-dev libfaac0 fdkaac  libfdk-aac0 libfdk-aac0-dbg libfdk-aac-dev libtheora-dbg libtheora-dev libtheora-doc libtheora0 libtheora-bin libtheora-ocaml libtheora-ocaml-dev libvpx-dev libvpx-doc libvpx3 libvpx3-dbg libwebp-dev libwebp5 libwebpdemux1 libwebpmux1 opencl-headers mesa-vdpau-drivers libvdpau-va-gl1 vdpauinfo vdpau-va-driver libvdpau-doc libvdpau-dev libvdpau1 libvdpau1-dbg libgsm-tools libgsm0710-0 libgsm0710-dev libgsm0710mux3 libgsm1 libgsm1-dbg libgsm1-dev sox libsox-dev libsox-fmt-all libsox-fmt-alsa libsox-fmt-ao libsox-fmt-base libsox-fmt-mp3 libsox-fmt-oss libsox-fmt-pulse libsox2 libsoxr-dev libsoxr-lsr0 libschroedinger-dev libschroedinger-doc libschroedinger-ocaml libschroedinger-ocaml-dev libschroedinger-1.0-0 libsmbclient libsmbclient-dev  smbclient  libspeex-dev libspeex1 libspeexdsp-dev libspeexdsp1 libspeex-ocaml libspeex-ocaml-dev libspeex-dbg libssh-4 libssh-dev libssh-dbg libssh-doc  libssh-gcrypt-4 libssh2-1 libssh2-1-dev libwavpack-dev libwavpack1 libxvidcore-dev libxvidcore4  libx265-dev libx265-79 libx265-doc libx264-148 libx264-dev libnetcdf-* netcdf-* libopenal-* openal-info  openssl 


When done, you may then create and load the appropriate environment modules for both ffmpeg and libav as your choices go. Don't load both at the same time, though :-) (Mark them as module conflicts to ensure that if this is set up on a cluster, library conflicts do not occur when users inadvertently load both of them by accident in the same session).

Now, we get to the interesting bits:

Encoding with VAAPI
-------------------

You'll notice that we pass several arguments to ffmpeg as indicated below:

    ffmpeg -loglevel debug -hwaccel vaapi -vaapi_device /dev/dri/renderD128 -i "input
    file" -vf 'format=nv12,hwupload' -map 0:0 -map 0:1 -threads 8 -aspect
    16:9 -y -f matroska -acodec copy -b:v 12500k -vcodec h264_vaapi
    "output file"

Let's break down these arguments to their meaning:

(a) **.-loglevel** tells ffmpeg to log ffmpeg events as debug output. This
will be very verbose, and is completely optional. You can disregard
this.

(b). **-vaapi_device**: This is important. You must select a valid VAAPI
H/W context device to which you will upload textures to via hwupload,
formatted in the NV12 colorspace. This points to a /dev/dri/render*_
file on your Linux system.

(c). **-vf** : This is an inbuilt ffmpeg option that allows you to specify
codec options/arguments to be passed to our encoder, in this case,
h264_vaapi (Remember, we built this when we passed --enable-vaapi at
the configuration stage). Here, we tell ffmpeg to convert all textures
to one colorspace, **NV12** (As it's the one accepted by Intel's QuickSync
hardware encoder) and to also use hwupload, an ffmpeg intrinsic, that
tells the program to asynchronously copy the converted pixel data to VAAPI's surfaces.

(d). **- threads** : Specifies the number of threads that FFmpeg should
use. By default, use the number of logical processors available on
your processor here. On Intel processors that support Hyperthreading,
multiply the number of cores your processor has by 2.

(e). **-f** : Specifies the container format specification you can use.
This can be Matroska, webm, mp4, etc. Take your pick (as per your
container constraints).

(f). **-acodec**: Specifies the audio codec to use when transcoding the
video's audio stream. In the example given above, we use ffmpeg's
muxers to copy the audio stream as is, untouched.

(g). **-vcodec**: Selects the video encoder to use. In this case, we
selected h264_vaapi, our key point of interest here.

(h).**-hwaccel vaapi**: This instructs ffmpeg to use VAAPI based hardware accelerated decode (for supported codecs, see platform limits), and it can drastically lower the processor load during the process. Note that you should only use this option if your hardware supports hardware-accelerated decoding via VAAPI for the source fornat being encoded. 

(i). **Using the `vaapi_scaler` in the video filters:**  It is possible to use Intel's QuickSync hardware via VAAPI for resize and scaling (when up-or downscaling the input source to a higher or lower resolution), using a filter snippet such as the one shown below:

    vf 'format=nv12,hwupload,scale_vaapi=w=1920:h=1080'

You may specify a different resolution by changing the dimensions in `=w=` and `:h=` to suit your needs.

See an example of this filter snippet used above in the two-pass example in FFmpeg below.

(j). **-hwaccel_output_format** : This option should be used every time you declare the **-hwaccel** method as **vaapi** , so that the decode stage takes place entirely in hardware. This option generates decode output directly on VAAPI hardware surfaces, speeding up decode performance significantly.


You may confirm supported decode formats on your setup by running [vainfo](https://launchpad.net/ubuntu/xenial/+package/vainfo):

    vainfo

Sample output on a [Haswell](http://www.hardwarezone.com.sg/feature-haswell-4th-gen-intel-hd-graphics-alls-well-new-igp) testbed:

    libva info: VA-API version 0.39.0
    libva info: va_getDriverName() returns 0
    libva info: Trying to open /usr/lib/x86_64-linux-gnu/dri/i965_drv_video.so
    libva info: Found init function __vaDriverInit_0_39
    libva info: va_openDriver() returns 0
    vainfo: VA-API version: 0.39 (libva 1.7.0)
    vainfo: Driver version: Intel i965 driver for Intel(R) Haswell Mobile - 1.7.0
    vainfo: Supported profile and entrypoints
          VAProfileMPEG2Simple            :	VAEntrypointVLD
          VAProfileMPEG2Simple            :	VAEntrypointEncSlice
          VAProfileMPEG2Main              :	VAEntrypointVLD
          VAProfileMPEG2Main              :	VAEntrypointEncSlice
          VAProfileH264ConstrainedBaseline:	VAEntrypointVLD
          VAProfileH264ConstrainedBaseline:	VAEntrypointEncSlice
          VAProfileH264Main               :	VAEntrypointVLD
          VAProfileH264Main               :	VAEntrypointEncSlice
          VAProfileH264High               :	VAEntrypointVLD
          VAProfileH264High               :	VAEntrypointEncSlice
          VAProfileH264MultiviewHigh      :	VAEntrypointVLD
          VAProfileH264MultiviewHigh      :	VAEntrypointEncSlice
          VAProfileH264StereoHigh         :	VAEntrypointVLD
          VAProfileH264StereoHigh         :	VAEntrypointEncSlice
          VAProfileVC1Simple              :	VAEntrypointVLD
          VAProfileVC1Main                :	VAEntrypointVLD
          VAProfileVC1Advanced            :	VAEntrypointVLD
          VAProfileNone                   :	VAEntrypointVideoProc
          VAProfileJPEGBaseline           :	VAEntrypointVLD

Supported encode formats are appended with the `VAEntrypointEncSlice` fields, and all decode formats(s) for your SKU will be listed under the `VAEntryPointVLD` and `VAEntrypointVideoProc` fields.

To interpret the output above, we can learn that the Haswell SKU above supports VAAPI - based hardware-accelerated decode for H.264 Simple, Main and Stereo High profiles (I'd assume that the Stereo High profile infers to [H.264's Multi-view coding](https://www.hhi.fraunhofer.de/en/departments/vca/research-groups/image-video-coding/research-topics/mvc-extension-of-h264avc.html) encode mode, useful for encoding 3D Blurays and similar media, implying feature parity with Windows-based implementations where MVC encodes and decodes are supported by Intel QuickSync. Need to test that sometime).


The other arguments are pretty standard to FFmpeg and need no introduction :-)

You may also use extra options such as QP mode (for constant-rate quality encoding) with this codec in ffmpeg as shown:

    ffmpeg -loglevel debug -vaapi_device /dev/dri/renderD128 -i "input file" -vf 'format=nv12,hwupload' -map 0:0 -map 0:1 -threads 8 -aspect 16:9 -y -f matroska -acodec copy -vcodec h264_vaapi -qp 19 -bf 2 "output file"

Here, you'll notice that we've added a few extra options to the arguments passed to the selected video encoder, `h264_vaapi`, and they are as follows:

(a). **-qp**: This option selects Fixed QP of P frames, and is ignored if bit-rate is set instead. Particularly useful for CRF-based encodes where a constant quality is required without bit-rate constraints. For a standard reference, a QP value of **~18** gives an approximate visual quality value similar to lossless compression, and **going higher (~51) will give you way worse visual quality.**

(b). **-bf**: This option toggles the maximum number of B-frames (bi-directional) between P-(progressive) frames. You may pump this higher than the default (2) if your selected encoder profile is High or better. Recommended: Leave this at the default (2). 

In my tests, it's also possible to do **two-pass encoding** with this encoder (`h264_vaapi`) in ffmpeg, as illustrated in the example below:

    ffmpeg -loglevel debug -hwaccel vaapi -hwaccel_output_format vaapi -i "input-file" -vaapi_device /dev/dri/renderD129  -vf 'format=nv12,hwupload,scale_vaapi=w=1920:h=1080' -pass 1 -qp:v 19 -b:v 10.5M -c:v h264_vaapi -bf 4 -threads 4 -aspect 16:9 -an -y -f mp4 "/dev/null" && ffmpeg -loglevel debug -hwaccel vaapi -hwaccel_output_format vaapi -i "phfx4k.mkv" -vaapi_device /dev/dri/renderD129 -vf 'format=nv12,hwupload,scale_vaapi=w=1920:h=1080' -pass 2 -acodec copy -c:v h264_vaapi -bf 4 -qp:v 19 -b:v 10.5M -threads 4 -aspect 16:9 -y -f mp4 "output.mp4"

Let's break that down:

With ffmpeg (and libav also), you **must specify both passes**  *sequentially* (**-pass 1** and **-pass 2**) because ffmpeg does not reiterate over input files for multiple passes. Secondly, this allows the user to tune the two-pass encoding as he/she sees fit, for example, by skipping audio processing in the first pass (**-an**) and only copying/muxing the audio stream from the input file's container specification into the output file's container (**-acodec copy**), as illustrated in the examples above.

And now we move on to libav's options for a similar encode:

    avconv -v 55 -y -vaapi_device /dev/dri/renderD128 -hwaccel vaapi -hwaccel_output_format vaapi -i input.mkv \
    -c:a copy -vf 'format=nv12|vaapi,hwupload' -c:v h264_vaapi -bf 2 -b 12500k output.mkv

Let's break down these arguments to their meaning:

(a) **.-v** : This defines avconv's verbosity level. This one is completely optional, though its' regarded as good practice to leave it enabled and set to a reasonable verbosity level as desired for troubleshooting and diagnostics purposes.

(b). **-vaapi_device**: This is important. You must select a valid VAAPI H/W context device to which you will upload textures to via hwupload, formatted in the NV12 colorspace. This points to a /dev/dri/render*_ file on your Linux system.

(c). **-hwaccel**: This option allows you to select the hardware - based accelerated decoding to use for the encode session. In our case above, we are picking vaapi as this has a positive impact on encoder performance. A nice freebie.

(d). **-hwaccel_output_format** : This option should be used every time you declare the **-hwaccel** method as **vaapi** , so that the decode stage takes place entirely in hardware. This option generates decode output directly on VAAPI hardware surfaces, speeding up decode performance significantly.

(e).  **-vf** : This is an inbuilt libav option that allows you to specify video filter options to be passed to our encoder, in this case, *h264_vaapi* (Remember, we built this when we passed --enable-vaapi at the configuration stage). Here, we tell libav to convert all textures to one colorspace, **NV12** (As it's the one accepted by Intel's QuickSync hardware encoder) and to also use hwupload, a libav intrinsic, that tells the program to asynchronously copy the converted pixel data to VAAPI's surfaces. This argument also includes the hardware accelerated decode output format we requested earlier, raw VAAPI hardware surfaces.

(f). **-bf** : Specifies the bframe setting to use. Sane values for Intel 's Quick Sync encode hardware should be between 2 and 4. Test and report back.

(g). **-c:a**: Specifies the audio codec to use when transcoding the video's audio stream. In the example given above, we use libav's muxers to copy the audio stream as is, untouched.

(h). **-c:v**: Selects the video encoder to use. In this case, we selected *h264_vaapi*, our key point of interest here.
(i). **-b**: Selects the video stream's bitrate passed to the encoder, *h264_vaapi*.

You may see the original documentation on Libav's website [here](https://wiki.libav.org/Hardware/vaapi) on build instructions, using the alternate hevc_vaapi on supported hardware, encoder limitations, caveats, etc.

If all well according to plan, your video file should be encoded to H.264, muxed into the selected container and be done with.

See the screen-shot library [here](http://imgur.com/gallery/jTmMk).

Extra information:
------------------

You can always view the build configuration of your Ffmpeg  pipeline at any times by running:

For FFmpeg:

    lin@mjanja:~$ ffmpeg -buildconf
    ffmpeg version N-80785-g0fd76d7 Copyright (c) 2000-2016 the FFmpeg developers
      built with gcc 5.3.1 (Ubuntu 5.3.1-14ubuntu2.1) 20160413
      configuration: --enable-nonfree --enable-gpl --enable-version3 --enable-libass --enable-libbluray --enable-libmp3lame --enable-libopencv --enable-libopenjpeg --enable-libopus --enable-libfaac --enable-libfdk-aac --enable-libtheora --enable-libvpx --enable-libwebp --enable-opencl --enable-x11grab --enable-opengl --cpu=native --enable-nvenc --enable-vaapi --enable-vdpau --enable-ladspa --enable-libass --enable-libgsm --enable-libschroedinger --enable-libsmbclient --enable-libsoxr --enable-libspeex --enable-libssh --enable-libwavpack --enable-libxvid --enable-libx264 --enable-libx265 --enable-netcdf --enable-openal --enable-openssl --prefix=/apps/ffmpeg/git --enable-omx
      libavutil      55. 27.100 / 55. 27.100
      libavcodec     57. 48.101 / 57. 48.101
      libavformat    57. 40.101 / 57. 40.101
      libavdevice    57.  0.102 / 57.  0.102
      libavfilter     6. 46.102 /  6. 46.102
      libswscale      4.  1.100 /  4.  1.100
      libswresample   2.  1.100 /  2.  1.100
      libpostproc    54.  0.100 / 54.  0.100
    
      configuration:
        --enable-nonfree
        --enable-gpl
        --enable-version3
        --enable-libass
        --enable-libbluray
        --enable-libmp3lame
        --enable-libopencv
        --enable-libopenjpeg
        --enable-libopus
        --enable-libfaac
        --enable-libfdk-aac
        --enable-libtheora
        --enable-libvpx
        --enable-libwebp
        --enable-opencl
        --enable-x11grab
        --enable-opengl
        --cpu=native
        --enable-nvenc
        --enable-vaapi
        --enable-vdpau
        --enable-ladspa
        --enable-libass
        --enable-libgsm
        --enable-libschroedinger
        --enable-libsmbclient
        --enable-libsoxr
        --enable-libspeex
        --enable-libssh
        --enable-libwavpack
        --enable-libxvid
        --enable-libx264
        --enable-libx265
        --enable-netcdf
        --enable-openal
        --enable-openssl
        --prefix=/apps/ffmpeg/git
        --enable-omx


**On help and documentation:**

List all formats:

    ffmpeg -formats

Display options specific to, and information about, a particular muxer:

    ffmpeg -h muxer=matroska

Display options specific to, and information about, a particular demuxer:

    ffmpeg -h demuxer=gif

**Codecs (encoders and decoders):**

List all codecs:

    ffmpeg -codecs

List all encoders:

    ffmpeg -encoders

List all decoders:

    ffmpeg -decoders

Display options specific to, and information about, a particular encoder:

    ffmpeg -h encoder=mpeg4

Display options specific to, and information about, a particular decoder:

    ffmpeg -h decoder=aac

**Reading the results**

There is a key near the top of the output that describes each letter that precedes the name of the format, encoder, decoder, or codec:

    $ ffmpeg -encoders
    […]
    Encoders:
     V..... = Video
     A..... = Audio
     S..... = Subtitle
     .F.... = Frame-level multithreading
     ..S... = Slice-level multithreading
     ...X.. = Codec is experimental
     ....B. = Supports draw_horiz_band
     .....D = Supports direct rendering method 1
     ------
    […]
     V.S... mpeg4                MPEG-4 part 2

In this example V.S... indicates that the encoder mpeg4 is a Video encoder and supports *Slice-level multithreading.*



**Extra notes for AMD hardware supporting VCE:**

If you have a supported GCN+ AMD GPU running on Linux with the mesa driver stack, you may be able to use the AMD VCE Block via VAAPI with an example such as the one shown below:

    DRI_PRIME=1 LIBVA_DRIVER_NAME=radeonsi ffmpeg -hwaccel vaapi -vaapi_device /dev/dri/renderD128 -hwaccel_output_format vaapi \
    -framerate 30 -video_size 1920x1200 -f x11grab -i :0.0 -f pulse -ac 2 -i 1 \
    -vf 'format=nv12,hwupload' -threads 8 \
    -vcodec h264_vaapi -bf 0 -acodec pcm_s16le output.mkv

Where we capture from the screen via `x11grab` and the audio from a `pulseaudio` device.

You must set the `LIBVA_DRIVER_NAME` and the `DRI_PRIME=1` environment variables to `radeonsi` prior to using VAAPI on VCE, and ensure that the `-vaapi_device` points to the correct renderer.

Note that with AMD hardware, we generally disable B-Frame support as newer SKUs such as the RX 460/470/480 and their rebrands (Polaris-based) do not support B-Frames in H.264 encoding.  See [this](https://github.com/GPUOpen-LibrariesAndSDKs/AMF/issues/8) issue on Github for more details.













