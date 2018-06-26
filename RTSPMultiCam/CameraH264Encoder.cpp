//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma warning(disable : 4996)
#include "GlobalParam.h"
#include "CameraH264Encoder.h"
#include "libavutil/base64.h"
#include <ctime>

extern int THREAD_STATE;
extern CRITICAL_SECTION   THREAD_STATE_MUTEX;

namespace CameraStreaming
{
   CameraH264Encoder::CameraH264Encoder(int ncol, int nrow) :
      fEncCtx(nullptr), fSrcFrame(nullptr), fEncodingOpt(nullptr),
      fPackOutCnt(0), OnFrameFunc(nullptr), fFrameInCnt(0)
   {
      fMosaicCol = ncol;
      fMosaicRow = nrow;

      strncpy(fFileName, "video_output_temp", 31);
      fFileName[31] = 0;

      set_sub_pict(640, 480);
      set_frame_rate(15);
      set_bit_rate(5 * 1024 * 1024); //10M

      //av_dict_set(&fEncodingOpt, "preset", "fast", 0);
      //av_dict_set(&fEncodingOpt, "slice-max-size", "30000", 0);
      InitializeCriticalSectionAndSpinCount(&fOutQueLock, 100);

      fState = 0;
   };


   void CameraH264Encoder::finalise()
   {
      close_encoder();

      DeleteCriticalSection(&fOutQueLock);

      for (auto&& ipict : fSubPictMutex) {
         DeleteCriticalSection(&ipict);
      };
   };

   void CameraH264Encoder::receive_frame(DataPacket* pict, int ipict)
   {
      EnterCriticalSection(&(fSubPictMutex[ipict]));
      fSubPict[ipict].swap(*pict);
      fSubPictState[ipict] = 1;
      LeaveCriticalSection(&(fSubPictMutex[ipict]));
      fflush(stderr);
   };

   void CameraH264Encoder::setup_encoder(enum AVCodecID codec_id, const char *enc_name)
   {
      fState = 0;

      AVDictionary       *opt = nullptr;
      AVDictionaryEntry  *dict_entry = nullptr;
      char                str_tmp[128] = { 0 };
      int ret;

      fEncoder = avcodec_find_encoder_by_name(enc_name);
      if (fEncoder == nullptr) {
         wprintf("cannot find encoder '%s', use 'libx264'", enc_name);

         fEncoder = avcodec_find_encoder_by_name("libx264");
         if (fEncoder == nullptr) {
            eprintf("failed to find codec 'libx264'!\n");
            return;
         }

      }

      if (fEncCtx != nullptr) {
         avcodec_free_context(&fEncCtx);
         fEncCtx = nullptr;
      }

      fEncCtx = avcodec_alloc_context3(fEncoder);
      if (fEncCtx == nullptr) {
         eprintf("Could not allocate video codec context");
         return;
      }

      fEncCtx->bit_rate = fBitRate;
      fEncCtx->width = fMosaicCol * fSubPictWidth;
      fEncCtx->height = fMosaicRow * fSubPictHeight;

      fEncCtx->framerate = fFrameRate;
      fEncCtx->time_base = av_inv_q(fFrameRate);
      fEncCtx->pix_fmt = AV_PIX_FMT_NV12;

      fEncCtx->gop_size = 10;
      fEncCtx->max_b_frames = 1;

      // set AV_CODEC_FLAG_GLOBAL_HEADER will disable b_repeat_header in libx264
      fEncCtx->flags &= ~AV_CODEC_FLAG_GLOBAL_HEADER;

      av_dict_copy(&opt, fEncodingOpt, 0);

      ret = avcodec_open2(fEncCtx, nullptr, &opt);
      if (ret < 0) {
         eprintf("open codec failed!\n");
         av_dict_free(&opt);
         return;
      }
      if (opt != nullptr) {
         iprintf("the following options are not used:");
         dict_entry = nullptr;
         while (dict_entry = av_dict_get(opt, "", dict_entry, AV_DICT_IGNORE_SUFFIX)) {
            fprintf(stdout, "  '%s' = '%s'\n", dict_entry->key, dict_entry->value);
         }
         av_dict_free(&opt);
      }

      if (fSrcFrame != nullptr) {
         av_frame_free(&fSrcFrame);
         fSrcFrame = nullptr;
      };

      fSrcFrame = av_frame_alloc();
      if (fSrcFrame == nullptr) {
         eprintf("allocating input frame failed!");
         return;
      }

      fSrcFrame->width = fEncCtx->width; //Note Resolution must be a multiple of 2!!
      fSrcFrame->height = fEncCtx->height;
      fSrcFrame->format = fEncCtx->pix_fmt;

      ret = av_frame_get_buffer(fSrcFrame, MEM_ALIGN);

      if (ret < 0) {
         eprintf("allocating frame data buffer failed!");
         return;
      }

      av_frame_make_writable(fSrcFrame);

      for (int iy = 0; iy < fSrcFrame->height; iy++) {
         for (int ix = 0; ix < fSrcFrame->width; ix++) {
            fSrcFrame->data[0][iy * fSrcFrame->linesize[0] + ix] = ix + iy + 3;
         }
      }

      /* Cb and Cr */
      for (int iy = 0; iy < fSrcFrame->height / 2; iy++) {
         for (int ix = 0; ix < fSrcFrame->width; ix+=2) {
            fSrcFrame->data[1][iy * fSrcFrame->linesize[1] + ix] = 128 + iy;
            fSrcFrame->data[1][iy * fSrcFrame->linesize[1] + ix + 1] = 64 + ix;
         }
      }

      fFrameInCnt = 0;
      //send dummy data to encoder
      AVPacket* pkt = av_packet_alloc();
      if (pkt == nullptr) {
         eprintf("allocating packet (av_packet_alloc()) failed!");
         return;
      }

      while (fPPSNAL.empty() || fSPSNAL.empty()) {
         //iprintf("send a packet to encoder!");
         fSrcFrame->pts = fFrameInCnt;
         ret = avcodec_send_frame(fEncCtx, fSrcFrame);

         if (ret != 0 && ret != AVERROR(EAGAIN)) {
            eprintf("sending frame for encoding error!");
            return;
         }

         ++fFrameInCnt;
         ret = 0;
         while (ret >= 0) {
            ret = avcodec_receive_packet(fEncCtx, pkt);
            if (ret != 0) {
               if (ret != AVERROR(EAGAIN)) {
                  eprintf("receiving encoded packet failed!");
               }
               break;
            }
            if (pkt->size < 4) continue;
            const uint8_t* pkt_bgn = pkt->data;
            const uint8_t* pkt_end = pkt_bgn + pkt->size;
            const uint8_t* pbgn = pkt_bgn;
            const uint8_t* pend = pkt_end;
            while (pbgn < pkt_end) {
               pbgn = find_next_nal(pbgn, pkt_end);
               if (pbgn == pkt_end) {
                  continue;
               }
               else
               {
                  while (*pbgn == 0) ++pbgn; //skip all 0x00
                  ++pbgn; //skip 0x01
               }
               pend = find_next_nal(pbgn, pkt_end);

               if (((*pbgn) & 0x1F) == 0x07) {    //SPS NAL
                  fSPSNAL.copy(pbgn, static_cast<int>(pend - pbgn));
               }

               if (((*pbgn) & 0x1F) == 0x08) {    //PPS NAL
                  fPPSNAL.copy(pbgn, static_cast<int>(pend - pbgn));
               }
               pbgn = pend;
            }
            send_packet(pkt);
         }
      }

      fFrameIntvl = (std::clock_t)(CLOCKS_PER_SEC / av_q2d(fFrameRate));
      //iprintf("fFrameIntvl = %d", fFrameIntvl);

      fPackOutCnt = 0;

      int npict = fMosaicCol * fMosaicRow;

      fSubPict.resize(npict);

      fSubPictState.clear();
      fSubPictState.resize(npict, 0);

      if (!fSubPictMutex.empty()) {
         for (auto & mutex : fSubPictMutex) {
            DeleteCriticalSection(&mutex);
         }
      }
      fSubPictMutex.resize(npict);

      for (auto & mutex : fSubPictMutex) {
         InitializeCriticalSectionAndSpinCount(&mutex, 1000);
      }
      fState = 1;
   };

   void CameraH264Encoder::run()
   {
      int ret;
      clock_t clk_pre = std::clock() - fFrameIntvl;
      clock_t clk_cur;
      std::vector<unsigned int> frame_cnt(GlobalParam::sub_pict_num(), 0);
      clock_t intvl = CLOCKS_PER_SEC / 1000;

      double clk2msec = 1000 / (double)CLOCKS_PER_SEC;
      long int msec;
      AVPacket* pkt = av_packet_alloc();
      if (pkt == nullptr) {
         eprintf("allocating packet (av_packet_alloc()) failed!");
         return;
      }

      const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(AV_PIX_FMT_NV12);

      AVFrame* frame = av_frame_alloc();
      if (frame == nullptr) {
         eprintf("allocating frame (av_frame_alloc()) failed!");
         return;
      }
      frame->width = fSubPictWidth;
      frame->height = fSubPictHeight;
      frame->format = AV_PIX_FMT_NV12;

      int dataInSz = av_image_get_buffer_size(AV_PIX_FMT_NV12, fSubPictWidth,
         fSubPictHeight, MEM_ALIGN);
      /*
      ret = av_frame_get_buffer(frame, MEM_ALIGN);
      if (ret < 0) {
         eprintf("allocating frame data buffer failed!");
         return;
      }
      */
      DataPacket pack_in(dataInSz);

      DataPacket pack_out;

      //Draw the frame line

      for (int ipict = 0; ipict < fSubPict.size(); ++ipict) {
         int pict_col = ipict % fMosaicCol;
         int pict_row = ipict / fMosaicCol;

         size_t x_shift = pict_col * frame->linesize[0];
         size_t y_shift = pict_row * frame->height;
         size_t pos;

         for (int iy = 0; iy < frame->height; ++iy) {
            pos = (iy + y_shift) * fSrcFrame->linesize[0] + x_shift;
            if (pict_col != 0) {
               fSrcFrame->data[0][pos] = 10;
            }
            if (pict_col != fMosaicCol - 1) {
               fSrcFrame->data[0][pos + frame->linesize[0] - 1] = 10;
            }
            if ((pict_row != 0 && iy == 0) || (pict_row != fMosaicRow - 1 && iy == frame->height - 1)) {
               memset(&(fSrcFrame->data[0][pos]), 10, frame->linesize[0]);
            }
         }
      }

      //do the loop
      while (true) {
         if (THREAD_STATE > 0) {
            break;
         }

         for (int ipict = 0; ipict < fSubPict.size(); ++ipict) {
            if (fSubPictState[ipict] == 1) {
               if (fSubPict[ipict].size() != dataInSz) {
                  wprintf("incoming data size is not right! in_data_sz = %d, frame_data_sz = %d",
                     fSubPict[ipict].size(), dataInSz);
               }

               //iprintf("picture %d is copied!", ipict);
               if (TryEnterCriticalSection(&(fSubPictMutex[ipict])) == 0) {
                  Sleep(1);
                  if (TryEnterCriticalSection(&(fSubPictMutex[ipict])) == 0) {
                     continue;
                  }
               }

               //iprintf("enter critical section - consume!");
               pack_in.swap(fSubPict[ipict]);
               fSubPictState[ipict] = 0;
               LeaveCriticalSection(&(fSubPictMutex[ipict]));
               //iprintf("leave critical section - consume!");

               av_image_fill_arrays(frame->data, frame->linesize, pack_in.data_ptr(),
                  (enum AVPixelFormat)frame->format, frame->width, frame->height, MEM_ALIGN);

               ++frame_cnt[ipict];

               int pict_col = ipict % fMosaicCol;
               int pict_row = ipict / fMosaicCol;

               int x_shift = pict_col * frame->linesize[0];
               int y_shift = pict_row * frame->height;

               //iprintf("0: ipict = %d, x_shift = %d, yshift = %d", ipict, x_shift, y_shift);

               uint8_t* src_ptr;
               uint8_t* dst_ptr;
               for (int iy = 0; iy < frame->height; ++iy) {
                  dst_ptr = &(fSrcFrame->data[0][(iy + y_shift) * fSrcFrame->linesize[0] + x_shift]);
                  src_ptr = &(frame->data[0][iy*frame->linesize[0]]);
                  memcpy(dst_ptr, src_ptr, frame->linesize[0]);
               }

               x_shift = pict_col * frame->linesize[1];
               y_shift = pict_row * frame->height / 2;
               //iprintf("1&2: ipict = %d, x_shift = %d, yshift = %d", ipict, x_shift, y_shift);
               for (int iy = 0; iy < frame->height / 2; ++iy) {
                  dst_ptr = &(fSrcFrame->data[1][(iy + y_shift) * fSrcFrame->linesize[1] + x_shift]);
                  src_ptr = &(frame->data[1][iy*frame->linesize[1]]);
                  memcpy(dst_ptr, src_ptr, frame->linesize[1]);
               }
            }
         }

         ////
         fSrcFrame->pts = fFrameInCnt;
         fSrcFrame->display_picture_number = static_cast<int>(fFrameInCnt % 0x7FFFFFFF);

         //send the picture at time interval
         clk_cur = std::clock();
         if (clk_cur - clk_pre + intvl < fFrameIntvl) {
            msec = static_cast<long>(clk2msec * (clk_cur - clk_pre + intvl));
            if (msec > 0 && msec < fFrameIntvl) {
               //wprintf("sleep for %d msec. ", msec);
               Sleep(msec);
            }
         }
         clk_pre = clk_cur;

         ret = avcodec_send_frame(fEncCtx, fSrcFrame);

         if (ret != 0 && ret != AVERROR(EAGAIN)) {
            eprintf("sending frame for encoding error!");
            return;
         }
         //iprintf("send a packet to encoder!");

         ++fFrameInCnt;

         ret = 0;
         while (ret >= 0) {
            ret = avcodec_receive_packet(fEncCtx, pkt);
            if (ret != 0) {
               if (ret != AVERROR(EAGAIN)) {
                  eprintf("receiving encoded packet failed!");
               }
               break;
            }

            send_packet(pkt);
         }
      }
      av_frame_free(&frame);
      av_packet_free(&pkt);
   };

   void CameraH264Encoder::send_packet(AVPacket* avpkt)
   {
      static DataPacket data_out;

      uint8_t *ptr = avpkt->data;
      int pkt_len = avpkt->size;
      if (ptr[0] == 0x00 && ptr[1] == 0x00) {
         if (ptr[2] == 0x01) {
            ptr += 3;
            pkt_len -= 3;
         }
         else if ((ptr[2] == 0x00) && (ptr[3] == 0x01)) {
            ptr += 4;
            pkt_len -= 4;
         }
      }

      if (pkt_len <= 0)
         return;

      data_out.copy(ptr, pkt_len);
      //iprintf("get a encoded data package start at %d end at %d", pbgn - pkt->data, pend - pkt->data);

      EnterCriticalSection(&fOutQueLock);
      if (fOutQuePos.full()) {
         fOutQuePos.pop_front();
      }
      fvOutQueue[fOutQuePos.push_back()].swap(data_out);
      LeaveCriticalSection(&fOutQueLock);
      if (OnFrameFunc != nullptr) {
         OnFrameFunc();
      }
   };

   void CameraH264Encoder::close_encoder()
   {

      if (fEncodingOpt) {
         av_dict_free(&fEncodingOpt);
      }

      if (fEncCtx != nullptr) {
         avcodec_free_context(&fEncCtx);
         fEncCtx = nullptr;
      }

      if (fSrcFrame != nullptr) {
         av_frame_free(&fSrcFrame);
         fSrcFrame = nullptr;
      };

   };

   void CameraH264Encoder::close_video()
   {
      close_encoder();
   };

   char CameraH264Encoder::fetch_packet(uint8_t** FrameBuffer, unsigned int* FrameSize)
   {
      if (!fOutQuePos.empty())
      {
         EnterCriticalSection(&fOutQueLock);
         size_t pos = fOutQuePos.front();
         fOutPack.swap(fvOutQueue[pos]);
         fOutQuePos.pop_front();
         LeaveCriticalSection(&fOutQueLock);

         fOutPack.set_ref_id(1);
         ++fPackOutCnt;
         *FrameBuffer = fOutPack.data_ptr();
         *FrameSize = fOutPack.size();
         //iprintf("a frame is delivered to RTSP server! sz = %d", *FrameSize);
         return 1;
      }
      else {
         *FrameBuffer = nullptr;
         *FrameSize = 0;
         return 0;
      }
   };

   char CameraH264Encoder::release_packet()
   {
      fOutPack.set_ref_id(0);
      return 1;
   }

   const uint8_t* CameraH264Encoder::find_next_nal(const uint8_t* start, const uint8_t* end)
   {
      const uint8_t *p = start;

      /* Simply lookup "0x000001" pattern */
      while (p <= end - 3) {
         if (p[0] == 0x00) {
            if (p[1] == 0x00) {
               if ((p[2] == 0x01) || (p[2] == 0x00 && p[3] == 0x01)) {
                  return p;
               }
               else {
                  p += 3;
               }
            }
            else {
               p += 2;
            }
         }
         else {
            ++p;
         }
      }

      return end;
   }
}
