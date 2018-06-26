# RTSPMultiCam

A RTSP server for streaming the combined picutre from mutiple webcams.

It reads pictures from mutiple webcam (dshow device) using FFMPEG library, combined them into a mosaic picuture, 
and streaming them out through unicast RTSP server using live555 library.

The program reads a settings from config.txt.

It assume the FFMPEG library and live555 library located at
- FFMPEG: 
  - header files ../FFMPEG/include
  - lib files ../FFMPEG/lib

- live555: 
  - library
   - ../live555/groupsock
   - ../live555/liveMedia
   - ../live555/UsageEnvironment
   - ../BasicUsageEnvironment
 - include
   - ../live555/groupsock/include
   - ../live555/liveMedia/include
   - ../live555/UsageEnvironment/include
   - ../BasicUsageEnvironment/include
 
## Credits:
- FFMPEG: www.ffmpeg.org
- live555: www.live555.com
- FFMPEG-Live555-H264-H265-Streamer: github.com/alm4096/FFMPEG-Live555-H264-H265-Streamer

## Author
Please address any correspondance to Jiaxin Du, jiaxin.du__at__hotmail.com
