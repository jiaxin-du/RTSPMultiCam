# RTSPMultiCam

## Introduction
![running the program](https://github.com/jiaxin-du/RTSPMultiCam/blob/master/cmd.jpg)
![playing in VLC](https://github.com/jiaxin-du/RTSPMultiCam/blob/master/playing.jpg)
A RTSP server for streaming the combined picutre from mutiple webcams.

It reads pictures from mutiple webcams (dshow devices) using FFMPEG library, combined them into a mosaic picuture, 
and streaming them out through unicast RTSP server using live555 library.

The program reads a settings from config.txt.
    
For your convenience, copies of FFMPEG and live55 library and header files are also included.

## Advantages:
- Read camera configuration (device name, device number, framerate) from a text file (see an exmaple in config.txt); 
- Select encoder and set its parameters from the text file, no need to reprogram;
- Separate threads for camera reading (one for each camera), video encoding and RTSP services, minimize the video delay.

## Credits:
- FFMPEG: www.ffmpeg.org
- live555: www.live555.com
- FFMPEG-Live555-H264-H265-Streamer: www.github.com/alm4096/FFMPEG-Live555-H264-H265-Streamer

## Author
Please address any correspondance to Jiaxin Du, jiaxin.du__at__hotmail.com
