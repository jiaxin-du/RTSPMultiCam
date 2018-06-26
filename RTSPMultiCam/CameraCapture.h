//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
// Credits:
// FFMPEG: http://www.ffmpeg.org
// live555: http://www.live555.com
// 
//***********************************************************
#pragma once

#include "misc.h"
#include "GlobalParam.h"

extern "C" {
#include <libavutil\mathematics.h>
#include <libavutil\imgutils.h>
#include <libswscale\swscale.h>
#include <libavutil\pixdesc.h>
#include <libavdevice\avdevice.h>
}

#include "DataPacket.h"

namespace CameraStreaming
{
   class CameraCapture
   {
   public:
      CameraCapture(const TCameraDesc *desc = nullptr);

      ~CameraCapture() {
         finalise();
      };

      void set_cam_desc(const TCameraDesc *cam) {
         fCamDesc = cam;
      }

      void set_callback_func(std::function<void(DataPacket *)> func) {
         ffOnFrameFunc = func;
      };

      bool ready() const {
         return (fState == 1) && (ffOnFrameFunc != nullptr);
      }

      int initialise();

      void capture();

      void finalise();

      void process_frame(AVFrame*, const uint64_t&);

   private:
      std::function<void(DataPacket *)> ffOnFrameFunc;

      AVFormatContext*    fInputCtx;
      std::clock_t        fDelayClock;
      AVCodecContext*     fDecCtx;
      AVDictionary*       fpCamInDict;
      SwsContext*         fpSwsCtx;
      AVFrame*            fFrameOut;
      DataPacket          fDataOut;
      uint64_t            fFrameCnt;
      uint64_t            fBitrate;
      int                 fDataOutSz;
      int                 fOutStreamNum;
      int                 fState;
      const TCameraDesc*  fCamDesc;

   };
}
