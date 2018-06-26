//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#pragma once

#include "misc.h"
#include <vector>

#define MAX_SUBPICT_COL    (3)
#define MAX_SUBPICT_ROW    (3)
#define DEFAULT_RTSP_PORT  (554)
#define DEFAULT_HTTP_PORT  (80)

class TCameraDesc {
public:
   enum EPict_opt {
      kPict_Mark_Time_stamp = 1,
      kPict_Mark_Spin_wheel = 2,
      kPict_Mark_horz_pos = 4,
      kPict_Mark_vert_pos = 8,
      kPict_First_col = 16,
      kPict_Last_col = 32,
      kPict_First_row = 64,
      kPict_Last_row = 128
   };

public:
   TCameraDesc(const std::string& xname = "", const int& xdevice_num = 0) :
      name(xname), pixel_format("YUYV422"), pin_name(""), pict_label(""),
      width(640), height(480), device_num(xdevice_num),
      rtbuffsize(1228800), /* buffer size for two copies of 640x480 yuyv420 pictures)*/
      frame_rate(20), options(1) /*time stamp is on by default*/
   { };

   ~TCameraDesc() = default;

   void set_option(unsigned int opt) {
      options |= opt;
   };

   void clr_option(unsigned int opt) {
      options &= ~opt;
   };

   std::string        name;             // The name of the camera in the system
   std::string        pixel_format;     // pixel format, will pass directly to ffmpeg 
   std::string        pin_name;         // pin number default number is zero 
   std::string        pict_label;        // set a mark on the picture
   uint64_t            rtbuffsize;       // real time buffer size, in byte
   double             frame_rate;       // frame rate  
   unsigned int       options;          // options
   int                width;            // picture size
   int                height;           // picture height
   int                device_num;       // device number, default value is zero
};

namespace CameraStreaming
{

   class GlobalParam
   {
   private:
      static char          fParamUnset[8];

      static TCameraDesc   fCamDesc[MAX_SUBPICT_COL * MAX_SUBPICT_ROW];

      static int           fSubPictWidth;
      static int           fSubPictHeight;

      static int           fMosaicCol;
      static int           fMosaicRow;
      static int           fSubPictNum;

      static char          fEncName[16];
      static std::vector<TParamPair>   fEncParam;
      static double        fOutFrameRate;
      static uint64_t       fOutBitRate;

      static int           fRTSPUDPPort;
      static int           fRTSPHTTPPort;
   public:

      static const TCameraDesc* cam_desc(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return NULL;
         }
         return &(fCamDesc[idx]);
      };

      static void set_cam_option(int idx, uint32_t opt) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return ;
         }
         fCamDesc[idx].options |= opt;
      };

      static void clr_cam_option(int idx, uint32_t opt) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return ;
         }
         fCamDesc[idx].options &= ~opt;
      };

      static uint32_t cam_option(const int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return NULL;
         }
         return fCamDesc[idx].options;
      };

      static void set_cam_name(int idx, const char* cam) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         fCamDesc[idx].name = cam;
      };

      static const char* cam_name(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return NULL;
         }
         return fCamDesc[idx].name.c_str();
      };

      static void set_cam_pin_name(int idx, const char* pin) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         fCamDesc[idx].pin_name = pin;
      };

      static const char* cam_pin_name(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return NULL;
         }
         return fCamDesc[idx].pin_name.c_str();
      };

      static void set_cam_fmt(int idx, const char* fmt) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         fCamDesc[idx].pixel_format = fmt;
      };

      static const char* cam_fmt(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return NULL;
         }
         return fCamDesc[idx].pixel_format.c_str();
      };

      static void set_cam_pict_label(int idx, const char* mark) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         fCamDesc[idx].pict_label = mark;
      };

      static const char* cam_pict_label(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return NULL;
         }
         return fCamDesc[idx].pict_label.c_str();
      };

      static void set_cam_dev_num(int idx, int num) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         fCamDesc[idx].device_num = num;
      };

      static int cam_dev_num(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return 0;
         }
         return fCamDesc[idx].device_num;
      };

      static void set_cam_buff_size(int idx, uint64_t sz) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         if (sz > 1 && sz < 102400LL) {
            iprintf("camera buffer size = %s is too small, set it to 1024 K", byte2str(sz).c_str());
            fCamDesc[idx].rtbuffsize = 102400LL;
         }
         else {
            fCamDesc[idx].rtbuffsize = sz;
         }
      };

      static uint64_t cam_buff_size(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return 0;
         }
         return  fCamDesc[idx].rtbuffsize;
      };

      static void set_cam_frame_rate(int idx, double sz) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return;
         }
         if (sz < 10) {
            iprintf("camera buffer size = %g is too small, set it to 10", sz);
            fCamDesc[idx].frame_rate = 10;
         }
         else if (sz > 30) {
            iprintf("camera buffer size = %g is too large, set it to 30", sz);
            fCamDesc[idx].frame_rate = 30;
         }
         else {
            fCamDesc[idx].frame_rate = sz;
         }
      };

      static double cam_frame_rate(int idx) {
         if (idx >= MAX_SUBPICT_COL * MAX_SUBPICT_ROW) {
            wprintf("camera index out of range!");
            return 0;
         }
         return fCamDesc[idx].frame_rate;
      };

      static void set_sub_pict_size(int w, int h) {
         set_sub_pict_width(w);
         set_sub_pict_height(h);
      };

      static void set_sub_pict_width(int w) {
         int itmp = (w + 3) & (~3U);
         if (itmp != w) {
            wprintf("picture width %d is invalid, change it to %d", w, itmp);
         }
         fSubPictWidth = itmp;

         for (int idx = 0; idx < MAX_SUBPICT_COL * MAX_SUBPICT_ROW; ++idx) {
            fCamDesc[idx].width = fSubPictWidth;
         }
      };

      static int sub_pict_width(void) {
         return fSubPictWidth;
      };

      static void set_sub_pict_height(int h) {
         int itmp = (h + 3) & (~3U);
         if (itmp != h) {
            wprintf("picture height %d is invalid, change it to %d", h, itmp);
         }
         fSubPictHeight = itmp;
         for (int idx = 0; idx < MAX_SUBPICT_COL * MAX_SUBPICT_ROW; ++idx) {
            fCamDesc[idx].height = fSubPictHeight;
         }
      };

      static int sub_pict_height(void) {
         return fSubPictHeight;
      };

      static void set_mosaic_col(int c) {
         if (c <= 0) {
            wprintf("subpict array column = %d is invalid, set it to 1!", c);
            fMosaicCol = 1;
         }
         else if (c > MAX_SUBPICT_COL) {
            wprintf("subpict array column = %d is too large, set it to %d!", c, MAX_SUBPICT_COL);
            fMosaicCol = MAX_SUBPICT_COL;
         }
         else {
            fMosaicCol = c;
         }

         fSubPictNum = fMosaicCol * fMosaicRow;
      };

      static int mosaic_col(void) {
         return fMosaicCol;
      };

      static void set_mosaic_row(int r) {
         if (r <= 0) {
            wprintf("subpict array row = %d is invalid, set it to 1!", r);
            fMosaicRow = 1;
         }
         else if (r > MAX_SUBPICT_ROW) {
            wprintf("subpict array row = %d is too large, set it to %d!", r, MAX_SUBPICT_ROW);
            fMosaicRow = MAX_SUBPICT_ROW;
         }
         else {
            fMosaicRow = r;
         }

         fSubPictNum = fMosaicCol * fMosaicRow;
      };

      static int mosaic_row(void) {
         return fMosaicRow;
      };

      static int sub_pict_num(void) {
         return fSubPictNum;
      };

      static void set_encoder_name(const char * enc) {
         strncpy_s(fEncName, enc, 15);
         fEncName[15] = 0;
      };

      static const char* encoder_name() {
         return fEncName;
      };

      static void set_encoder_param(const std::string& name, const std::string& val) {
         for (auto&& param : fEncParam) {
            if (name == param.name) {
               param.val = val;
               iprintf("encoder parameter '%s' set twice, use value '%s'!", name.c_str(), val.c_str());
               return;
            }
         }
         
         fEncParam.push_back(TParamPair(name, val));
      };

      static int encoder_param_num(void) {
         return static_cast<int>(fEncParam.size());
      };

      static const char* encoder_param_name(int idx) {
         if (idx < 0 || idx >= fEncParam.size()) {
            wprintf("idx out of range!");
            return fParamUnset;
         }
         return fEncParam[idx].name.c_str();
      };

      static const char* encoder_param_val(int idx) {
         if (idx < 0 || idx >= fEncParam.size()) {
            wprintf("idx out of range!");
            return fParamUnset;
         }
         return fEncParam[idx].val.c_str();
      };

      static void set_frame_rate(double fps) {
         fOutFrameRate = fps;
      }

      static double frame_rate(void) {
         return fOutFrameRate;
      };

      static void set_bit_rate(uint64_t br) {
         fOutBitRate = br;
      }

      static uint64_t bit_rate(void) {
         return fOutBitRate;
      };

      static void set_RTSP_port(int num) {
         if (num < 10 || num > 65535) {
            wprintf("invalid port number %d, set to %d", num, DEFAULT_RTSP_PORT);
            fRTSPUDPPort = DEFAULT_RTSP_PORT;
         }
         fRTSPUDPPort = num;
      }

      static int RTSP_port(void) {
         return fRTSPUDPPort;
      };

      static void set_HTTP_port(int num) {
         if (num < 10 || num > 65535) {
            wprintf("invalid port number %d, set to %d", num, DEFAULT_HTTP_PORT);
            fRTSPHTTPPort = DEFAULT_HTTP_PORT;
         }
         fRTSPHTTPPort = num;
      }

      static int HTTP_port(void) {
         return fRTSPHTTPPort;
      };

      static bool read_from_file(const char*);

      static void print();
   public:
      GlobalParam();
      ~GlobalParam();
   };
}
