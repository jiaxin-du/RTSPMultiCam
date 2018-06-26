//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#pragma warning(disable : 4996)
#include "CameraCapture.h"

extern int THREAD_STATE;
extern CRITICAL_SECTION   THREAD_STATE_MUTEX;

namespace CameraStreaming
{
   CameraCapture::CameraCapture(const TCameraDesc * desc) :
      ffOnFrameFunc(nullptr), fInputCtx(nullptr), fDecCtx(nullptr),
      fpCamInDict(nullptr), fpSwsCtx(nullptr), fFrameOut(nullptr), fBitrate(5120000),
      fFrameCnt(0), fState(0), fCamDesc(desc), fDelayClock(CLOCKS_PER_SEC / 15)
   {

   };

   int CameraCapture::initialise()
   {
      fState = 0;

      if (fCamDesc == NULL) {
         wprintf("camera description pointer not set!");
         return -1;
      }

      if (fCamDesc->name[0] == '\0') {
         wprintf("camera name is empty!");
         return -1;
      }

      int ret = 0;

      char str_tmp[128];
      str_tmp[127] = 0;

      AVDictionary* opt = nullptr;
      AVDictionaryEntry* dict_entry = nullptr;
      AVInputFormat *inFrmt = nullptr;

      // Initialize FFmpeg environment
      av_register_all();
      avcodec_register_all();
      avdevice_register_all();
      av_log_set_level(AV_LOG_ERROR);
      //avformat_network_init();

      if (fDecCtx != nullptr) {
         avcodec_free_context(&fDecCtx);
         fDecCtx = nullptr;
      }

      if (fInputCtx != nullptr) {
         avformat_close_input(&fInputCtx);
         fInputCtx = nullptr;
      }

      if (fFrameOut != nullptr) {
         if (fFrameOut->data[0] != nullptr) {
            av_freep(fFrameOut->data[0]);
         }
         av_frame_free(&fFrameOut);
         fFrameOut = nullptr;
      }

      fInputCtx = avformat_alloc_context();

      inFrmt = av_find_input_format("dshow");

      //video size
      ret = snprintf(str_tmp, 127, "video_size=%dx%d;pixel_format=%s;framerate=%g;",
         fCamDesc->width, fCamDesc->height, (fCamDesc->pixel_format).c_str(),
         fCamDesc->frame_rate);

      av_dict_parse_string(&opt, str_tmp, "=", ";", 0);

      if (fCamDesc->device_num > 0) {
         ret += av_dict_set(&opt, "video_device_number", int2str(fCamDesc->device_num).c_str(), 0);
      }
      if (!fCamDesc->pin_name.empty()) {
         av_dict_set(&opt, "video_pin_name", fCamDesc->pin_name.c_str(), 0);
      }

      if (fCamDesc->rtbuffsize > 102400) { //ignore a small rtbuffsize
         av_dict_set(&opt, "rtbufsize", byte2str(fCamDesc->rtbuffsize).c_str(), 0);
      }

      /*
      iprintf("open the camera '%s' with the options:", fCamDesc->name.c_str());
      dict_entry = NULL;
      while (dict_entry = av_dict_get(opt, "", dict_entry, AV_DICT_IGNORE_SUFFIX)) {
         fprintf(stdout, "  '%s' = '%s'\n", dict_entry->key, dict_entry->value);
      }
      */
      snprintf(str_tmp, 127, "video=%s", fCamDesc->name.c_str());

      if (ret = avformat_open_input(&fInputCtx, str_tmp, inFrmt, &opt) != 0) {
         //exception
         av_dict_free(&opt);
         av_strerror(ret, str_tmp, 127);
         eprintf("open video input '%s' failed! err = %s", fCamDesc->name.c_str(), str_tmp);
         return -1;
      }

      if (opt != nullptr) {
         iprintf("the following options are not used:");
         dict_entry = nullptr;
         while (dict_entry = av_dict_get(opt, "", dict_entry, AV_DICT_IGNORE_SUFFIX)) {
            fprintf(stdout, "  '%s' = '%s'\n", dict_entry->key, dict_entry->value);
         }
         av_dict_free(&opt);
      }

      if (avformat_find_stream_info(fInputCtx, nullptr) < 0) {
         //exception
         eprintf("getting audio/video stream information from '%s' failed!", fCamDesc->name.c_str());
         return -1;
      }

      //print out the video stream information
      iprintf("video stream from '%s': ", fCamDesc->name.c_str());
      av_dump_format(fInputCtx, 0, fInputCtx->url, 0);

      fOutStreamNum     = -1;
      AVStream*  stream = nullptr;
      for (unsigned int i = 0; i < fInputCtx->nb_streams; i++) {
         if (fInputCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            fOutStreamNum = i;
            stream = fInputCtx->streams[i];
         }
      }

      if (fOutStreamNum == -1 || stream == nullptr) {
         //exception
         eprintf("no video stream found in '%s'!\n", fCamDesc->name.c_str());
         return -1;
      }

      AVCodecParameters  *codec_param = stream->codecpar;

      fBitrate = codec_param->bit_rate;

      fDelayClock = (int)((double)(CLOCKS_PER_SEC) * (double)(stream->avg_frame_rate.den) / (double)(stream->avg_frame_rate.num));

      AVCodec* decoder = avcodec_find_decoder(codec_param->codec_id);
      if (decoder == nullptr) {
         eprintf("cannot find decoder for '%s'!", avcodec_get_name(codec_param->codec_id));
         return -1;
      }

      fDecCtx = avcodec_alloc_context3(decoder);

      avcodec_parameters_to_context(fDecCtx, codec_param);

      av_log_set_level(AV_LOG_FATAL);
      if (avcodec_open2(fDecCtx, decoder, nullptr) < 0) {
         wprintf("Cannot open video decoder for camera!");
         return -1;
      }

      fpSwsCtx = sws_getCachedContext(nullptr, fDecCtx->width, fDecCtx->height, fDecCtx->pix_fmt,
         fDecCtx->width, fDecCtx->height, AV_PIX_FMT_NV12, SWS_BICUBIC, nullptr, nullptr, nullptr);

      fFrameOut = av_frame_alloc();
      if (fFrameOut == nullptr) {
         eprintf("allocating output frame failed!");
         return -1;
      }
      fFrameOut->width = fDecCtx->width;
      fFrameOut->height = fDecCtx->height;
      fFrameOut->format = AV_PIX_FMT_NV12;
      fDataOutSz = av_image_get_buffer_size(AV_PIX_FMT_NV12, fFrameOut->width,
         fFrameOut->height, MEM_ALIGN);
      ret = av_frame_get_buffer(fFrameOut, MEM_ALIGN);
      if (ret < 0) {
         eprintf("allocating frame data buffer failed!");
         return -1;
      }

      fDataOut.resize(fDataOutSz);
      fState = 1;
      return 0;
   };


   void CameraCapture::process_frame(AVFrame* frame, const uint64_t& frame_cnt)
   {
      //set frame
      for (int iy = 0; iy < frame->height; ++iy) {
         if (!(fCamDesc->options & TCameraDesc::kPict_First_col)) {
            frame->data[0][iy * frame->linesize[0]] = 10;
         }
         if (!(fCamDesc->options & TCameraDesc::kPict_Last_col)) {
            frame->data[0][(iy + 1) * frame->linesize[0] - 1] = 10;
         }
      }

      if (!(fCamDesc->options & TCameraDesc::kPict_First_row)) {
         memset(frame->data[0], 10, frame->linesize[0]);
      }
      if (!(fCamDesc->options & TCameraDesc::kPict_Last_row)) {
         memset(frame->data[0] + (frame->height - 1)*frame->linesize[0], 10, frame->linesize[0]);
      }
      
      int x_shift, y_shift;
      if (fCamDesc->options & TCameraDesc::kPict_Mark_Spin_wheel) {
         if (fCamDesc->options & TCameraDesc::kPict_First_col) {
            x_shift = 1;
         }
         else {
            x_shift = frame->linesize[0] - 17;
         }

         if (fCamDesc->options & TCameraDesc::kPict_First_row) {
            y_shift = 1;
         }
         else {
            y_shift = frame->height - 17;
         }
         for (int iy = 0; iy < 16; ++iy) {
            for (int ix = 0; ix < 16; ++ix) {
               unsigned int pos = 2 * (iy / 8) + (ix / 8);
               if (pos == 3)
                  pos = 2;
               else if (pos == 2)
                  pos = 3;
               if (pos == ((frame_cnt >> 1) & 0x03)) {
                  frame->data[0][ix + x_shift + (iy + y_shift) * frame->linesize[0]] = 255;
               }
               else {
                  frame->data[0][ix + x_shift + (iy + y_shift) * frame->linesize[0]] /= 2;
               }
            }
         }
      }
      else if (fCamDesc->options & TCameraDesc::kPict_Mark_Time_stamp) {
         SYSTEMTIME st;

         GetLocalTime(&st);

         char str_tmp[128] = { 0 };
         str_tmp[127] = 0;
         int pos = 0;
         if (!(fCamDesc->pict_label).empty()) {
            pos = snprintf(str_tmp, 17, " %s", fCamDesc->pict_label.c_str());
         }
         snprintf(str_tmp + pos, 127 - pos, " %04d/%02d/%02d %02d:%02d:%02d ",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

         if (fCamDesc->options & TCameraDesc::kPict_First_col) {
            x_shift = 1;
         }
         else {
            x_shift = frame->linesize[0] - LETTER_WIDTH_PIXEL * static_cast<int>(strlen(str_tmp)) - 1;
            if (x_shift < 1) x_shift = 1;
         }

         if (fCamDesc->options & TCameraDesc::kPict_First_row) {
            y_shift = 1;
         }
         else {
            y_shift = frame->height - LETTER_HEIGHT_PIXEL - 1;
         }

         int icol = x_shift;
         int irow = y_shift;

         for (int ich = 0; ich < strlen(str_tmp); ++ich) {
            for (int iy = irow; iy < irow + LETTER_HEIGHT_PIXEL; ++iy) {
               int idx = static_cast<int>(str_tmp[ich]) - 32;
               if (idx < 0 || idx >= 95) idx = 32;
               unsigned int pixel_arry = pixel_array_letters[idx][irow + LETTER_HEIGHT_PIXEL - 1 - iy];
               for (int ix = icol; ix < icol + LETTER_WIDTH_PIXEL; ++ix) {
                  if (TEST_BIT(pixel_arry, icol + LETTER_WIDTH_PIXEL - 1 - ix)) {
                     frame->data[0][iy * frame->linesize[0] + ix] = 255;
                  }
                  else {
                     frame->data[0][iy * frame->linesize[0] + ix] /= 2;

                  }
               }
            }
            icol += LETTER_WIDTH_PIXEL;
         }
      }
   };

   void CameraCapture::capture()
   {
      if (fState == 0) return;

      int ret;

      int packet_cnt = 0;

      AVPacket*  packet = av_packet_alloc();
      AVFrame*  frame = av_frame_alloc();

      while (true) {
         if (THREAD_STATE > 0) break;

         ret = av_read_frame(fInputCtx, packet);
         if (ret < 0) {
            break;
         }

         ++packet_cnt;

         if (packet->buf == nullptr || packet->stream_index != fOutStreamNum)
            continue;

         ret = avcodec_send_packet(fDecCtx, packet);
         if (ret != 0 && ret != AVERROR(EAGAIN)) {
            eprintf("sending a packet for decoding failed!");
            return;
         }

         do {
            ret = avcodec_receive_frame(fDecCtx, frame);

            if (ret == AVERROR(EAGAIN)) {
               break;
            }
            else if (ret != 0) {
               eprintf("reading a frame from decoder failed!");
               return;
            }

            ret = sws_scale(fpSwsCtx, frame->data, frame->linesize, 0, frame->height,
               fFrameOut->data, fFrameOut->linesize);
            if (ret <= 0) {
               eprintf("scaling picture failed!");
               return;
            }
            process_frame(fFrameOut, fFrameCnt);

            fDataOut.resize(fDataOutSz);

            av_image_copy_to_buffer(fDataOut.data_ptr(), fDataOut.size(), fFrameOut->data,
               fFrameOut->linesize, (enum AVPixelFormat)fFrameOut->format, \
               fFrameOut->width, fFrameOut->height, MEM_ALIGN);

            //fDataOut.copy(fFrameOut->data[0], fDataOutSz);
            ++fFrameCnt;
            if (ffOnFrameFunc != nullptr) {
               ffOnFrameFunc(&fDataOut);
            }

            //iprintf("send a frame to encoder, size = %d.", frame->pkt_size);
         } while (ret >= 0);

         Sleep(1);

         av_packet_unref(packet);
         av_frame_unref(frame);
      }

      av_frame_free(&frame);
      av_packet_free(&packet);
   };

   void CameraCapture::finalise()
   {
      av_dict_free(&fpCamInDict);
      avcodec_free_context(&fDecCtx);
      avformat_close_input(&fInputCtx);
      if (fFrameOut) {
         av_frame_free(&fFrameOut);
      }
      sws_freeContext(fpSwsCtx);
      fpSwsCtx = nullptr;
   };

}



