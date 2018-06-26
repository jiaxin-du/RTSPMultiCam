# RTSPMultiCam

## Introduction
![running the program](https://github.com/jiaxin-du/RTSPMultiCam/blob/master/cmd.jpg)
![playing in VLC](https://github.com/jiaxin-du/RTSPMultiCam/blob/master/playing.jpg)
A RTSP server for streaming the combined picutre from mutiple webcams.

It reads pictures from mutiple webcams (dshow devices) using FFMPEG library, combined them into a mosaic picuture, 
and streaming them out through unicast RTSP server using live555 library.

The program reads a settings from config.txt.
    
## Advantages:
- Read camera configuration (device name, device number, framerate) from a text file (see an exmaple in config.txt); 
- Select encoder and set its parameters from the text file, no need to reprogram;
- Separate threads for camera reading (one for each camera), video encoding and RTSP services, minimize the video delay.

## How to compile
 - Download the source code;
 - Download FFMPEG library from www.ffmpeg.org, you need the development library;
 - Download live555 from www.live555.com and compile it following the intruction on the website;
 - Make sure the libaries are presented and located in the right directory;
 - Open the Visual Studio solution and compile it.
 - The program can be compiled in linux (ubuntu) with minor changes, you can figure out the steps.
 
## Credits:
- FFMPEG: www.ffmpeg.org
- live555: www.live555.com
- FFMPEG-Live555-H264-H265-Streamer: www.github.com/alm4096/FFMPEG-Live555-H264-H265-Streamer

## Author
Please address any correspondance to Jiaxin Du, jiaxin.du__at__hotmail.com
