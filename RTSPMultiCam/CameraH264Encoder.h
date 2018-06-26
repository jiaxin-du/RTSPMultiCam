//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#pragma once

#include "misc.h"
#include <queue>

extern "C" {
#include <libavutil\opt.h>
#include <libavutil\mathematics.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>
#include <libswresample\swresample.h>
#include <libavutil\imgutils.h>
#include <libavcodec\avcodec.h>
}

#include "DataPacket.h"

namespace CameraStreaming
{
   template<std::size_t sz>
   class TCircularQueue {
   private:
      const std::size_t fBuffSize;
      std::size_t fDataSize;
      std::size_t fDataFront;
      std::size_t fDataRear;
   public:
      TCircularQueue() :
         fDataSize(0), fBuffSize(sz), fDataFront(0), fDataRear(sz - 1)
      {
      };

      size_t size() const {
         return fDataSize;
      };

      size_t capacity() const {
         return fBuffSize;
      };

      bool full() const {
         return (fDataSize == fBuffSize);
      };

      bool empty() const {
         return (fDataSize == 0);
      };

      size_t push_back() {
         ++fDataRear;
         if (fDataRear == fBuffSize) {
            fDataRear = 0;
         }
         if (fDataSize == fBuffSize) {
            pop_front();
         }
         else {
            ++fDataSize;
         }
         return fDataRear;
      };

      size_t pop_front() {
         size_t pos = 0xFFFFFFFF;
         if (fDataSize > 0) {
            pos = fDataFront;
            ++fDataFront;
            if (fDataFront == fBuffSize) {
               fDataFront = 0;
            }
            --fDataSize;
         };
         return pos;
      };

      size_t front() const {
         return fDataFront;
      };

      size_t rear() const {
         return fDataRear;
      };
   };

   class CameraH264Encoder
   {
   public:
      static const char* codec;
   public:
      CameraH264Encoder(int ncol = 2, int nrow = 2);
      ~CameraH264Encoder() {
         finalise();
      };

      void finalise();

      void set_callback_func(std::function<void()> func)
      {
         OnFrameFunc = func;
      };

      void close_video();

      void setup_encoder(enum AVCodecID, const char*);

      void close_encoder();

      void receive_frame(DataPacket* pict, int ipict);

      char release_packet();

      void run();

      char fetch_packet(uint8_t** FrameBuffer, unsigned int* FrameSize);

      void set_sub_pict(int width, int height)
      {
         fSubPictWidth = width;
         fSubPictHeight = height;
      };

      int sub_pict_width() const {
         return fSubPictWidth;
      };

      int sub_pict_height() const {
         return fSubPictHeight;
      };

      int pict_width() const {
         return fMosaicCol * fSubPictWidth;
      };

      int pict_height() const {
         return fMosaicRow * fSubPictHeight;
      };

      void set_frame_rate(double fps) {
         fFrameRate = av_d2q(fps, 100);
      };

      double frame_rate(void) const {
         return av_q2d(fFrameRate);
      };

      void set_bit_rate(uint64_t br) {
         fBitRate = br;
      };

      void set_encoder_param(const char* name, const char* val) {
         if (name == NULL || name[0] == 0 || val == NULL || val[0] == NULL) {
            iprintf("setting encoder parameter failed: name or val is empty!");
         }
         av_dict_set(&fEncodingOpt, name, val, 0);
      };

      uint64_t h264_bit_rate() const {
         return fBitRate;
      };

      bool ready() const {
         return (fState == 1);
      }

      void send_packet(AVPacket*);
      static const uint8_t* find_next_nal(const uint8_t* pbgn, const uint8_t* pend);

      const uint8_t* PPS_NAL(void) {
         if (!fPPSNAL.empty())
            return fPPSNAL.data_ptr();
         return NULL;
      };

      int PPS_NAL_size(void) {
         return fPPSNAL.size();
      };

      const uint8_t* SPS_NAL(void) {
         if (!fSPSNAL.empty())
            return fSPSNAL.data_ptr();
         return NULL;
      };

      int SPS_NAL_size(void) {
         return fSPSNAL.size();
      };

   private:
      std::vector<DataPacket>        fSubPict;
      std::vector<CRITICAL_SECTION>  fSubPictMutex;
      std::vector<int>               fSubPictState;

      DataPacket                     fvOutQueue[32];
      TCircularQueue<32>             fOutQuePos;
      CRITICAL_SECTION               fOutQueLock;

      DataPacket                     fOutPack;
      AVDictionary*                  fEncodingOpt;
      AVCodecContext                 *fEncCtx;
      AVCodec                        *fEncoder;
      AVFrame                        *fSrcFrame;

      DataPacket                      fSPSNAL;
      DataPacket                      fPPSNAL;

      std::function<void()>           OnFrameFunc;

      AVRational                      fFrameRate;
      std::clock_t                    fFrameIntvl;
      uint64_t                        fBitRate;
      uint64_t                        fFrameInCnt;
      uint64_t                        fPackOutCnt;
      int	                          fSubPictWidth;
      int	                          fSubPictHeight;

      int                             fMosaicCol;
      int                             fMosaicRow;

      char                            fFileName[32];
      char                            fState;
   };
}
