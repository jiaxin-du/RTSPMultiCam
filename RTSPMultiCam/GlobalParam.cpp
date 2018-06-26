//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#include "GlobalParam.h"
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

using std::string;

namespace CameraStreaming {

   char          GlobalParam::fParamUnset[8] = { 'U', 'N', 'S', 'E', 'T', '\0' };

   TCameraDesc   GlobalParam::fCamDesc[MAX_SUBPICT_COL*MAX_SUBPICT_ROW];

   int           GlobalParam::fSubPictWidth = 640;
   int           GlobalParam::fSubPictHeight = 480;

   int           GlobalParam::fMosaicCol = 2;
   int           GlobalParam::fMosaicRow = 2;
   int           GlobalParam::fSubPictNum = 4;

   char          GlobalParam::fEncName[16] = { 'l', 'i', 'b', 'x', '2', '6', '4', '\0' }; //default libx264
   double        GlobalParam::fOutFrameRate = 15;    // 15 frame per sec
   uint64_t      GlobalParam::fOutBitRate = 5242880; // 5 MB

   std::vector<TParamPair>  GlobalParam::fEncParam;

   int           GlobalParam::fRTSPUDPPort = 554;
   int           GlobalParam::fRTSPHTTPPort = 80;

   GlobalParam::GlobalParam()
   { };

   GlobalParam::~GlobalParam()
   { };

   void GlobalParam::print()
   {
      printf("//picture information\n");
      printf("subpict_width = %d;\n", fSubPictWidth);
      printf("subpict_height = %d;\n", fSubPictHeight);

      printf("\n//mosaic information\n");
      printf("mosaic_col = %d;\n", fMosaicCol);
      printf("mosaic_row = %d;\n", fMosaicRow);

      printf("\n//encoder information\n");
      printf("encoder {\n");
      printf("  name = \"%s\";\n", fEncName);
      printf("  frame_rate = %g;\n", fOutFrameRate);
      printf("  bit_rate = %s;\n", byte2str(fOutBitRate).c_str());
      for (auto&& param : fEncParam) {
         printf("  %s = \"%s\";\n", param.name.c_str(), param.val.c_str());
      }
      printf("};\n");

      printf("\n//RTSP information\n");
      printf("RTSP_port = %d;\n", fRTSPUDPPort);
      printf("HTTP_port = %d;\n", fRTSPHTTPPort);

      printf("\n//camera information\n");

      for (int idx = 0; idx < sub_pict_num(); ++idx) {
         printf("\n//camera %d\n", idx);
         if (fCamDesc[idx].name[0] != '\0') {
            printf("cam_%d { \n", idx);
            printf("  name = \"%s\";\n", fCamDesc[idx].name.c_str());

            if (fCamDesc[idx].device_num > 0)
               printf("  device_num = %d;\n", fCamDesc[idx].device_num);

            if (!fCamDesc[idx].pin_name.empty()) 
               printf("  pin_name = %s;\n", fCamDesc[idx].pin_name.c_str());

            printf("  buff_size = %s;\n", byte2str(fCamDesc[idx].rtbuffsize).c_str());
            printf("  frame_rate = %g;\n", fCamDesc[idx].frame_rate);
            printf("  pixel_format = \"%s\";\n", fCamDesc[idx].pixel_format.c_str());

            if (!fCamDesc[idx].pict_label.empty()) {
               printf("  pict_label = \"%s\";\n", fCamDesc[idx].pict_label.c_str());
            }

            printf("};\n");
         }
         else {
            printf("//camera %d not in use!\n", idx);
         }
      }
   };

   bool GlobalParam::read_from_file(const char* fname)
   {
      double rval;
      int idx, ival;

      std::map <std::string, std::string> param_list;
      std::map <std::string, std::string>::iterator curr_it;
      std::map <std::string, std::string>::iterator it;
      string str;
      size_t pos;
      if (!read_param(fname, param_list)) {
         goto RETURN_ERROR;
      }

      curr_it = param_list.begin();
      it = param_list.begin();

      while (it != param_list.end()) {
         if (it->first == "subpict_width") {
            if (!str2int(it->second, ival)) {
               curr_it = it;
               goto PARAM_ERROR;
            }
            set_sub_pict_width(ival);
         }
         else if (it->first == "subpict_height") {
            if (!str2int(it->second, ival)) {
               curr_it = it;
               goto PARAM_ERROR;
            }
            set_sub_pict_height(ival);
         }
         else if (it->first == "mosaic_col") {
            if (!str2int(it->second, ival)) {
               curr_it = it;
               goto PARAM_ERROR;
            }
            set_mosaic_col(ival);
         }
         else if (it->first == "mosaic_row") {
            if (!str2int(it->second, ival)) {
               curr_it = it;
               goto PARAM_ERROR;
            }
            set_mosaic_row(ival);
         }

         else if (it->first == "RTSP_port") {
            if (!str2int(it->second, ival)) {
               curr_it = it;
               goto PARAM_ERROR;
            }
            set_RTSP_port(ival);
         }
         else if (it->first == "HTTP_port") {
            if (!str2int(it->second, ival)) {
               curr_it = it;
               goto PARAM_ERROR;
            }
            set_HTTP_port(ival);
         }
         else if (it->first.substr(0, 8) == "encoder.") {
            str = it->first.substr(8);
            if (str == "name") {
               if ((it->second).empty()) {
                  curr_it = it;
                  goto PARAM_ERROR;
               }
               set_encoder_name((it->second).c_str());
            }
            else if (str == "frame_rate") {
               if (!str2double(it->second, rval)) {
                  curr_it = it;
                  goto PARAM_ERROR;
               }
               set_frame_rate(rval);
            }
            else if (str == "bit_rate") {
               uint64_t lval;
               if (!str2byte(it->second, lval)) {
                  curr_it = it;
                  goto PARAM_ERROR;
               }
               set_bit_rate(lval);
            }
            else {
               if (str.empty() || (it->second).empty()) {
                  curr_it = it;
                  goto PARAM_ERROR;
               }
               set_encoder_param(str, it->second);
            }
         }
         else {
            ++it;
            continue;
         }

         param_list.erase(it);
         it = param_list.begin();
      }

      curr_it = param_list.begin();
      it = param_list.begin();
      while (it != param_list.end()) {
         if (it->first.substr(0, 4) == "cam_") {
            pos = it->first.find_first_of(".");
            if (pos == string::npos || !str2int(it->first.substr(4, pos - 4), ival)) {
               curr_it = it;
               iprintf("invalid camera num!");
               goto PARAM_ERROR;
            }
            idx = ival;
            str = it->first.substr(pos + 1);
            if (str == "name") {
               if (it->second.empty()) {
                  curr_it = it;
                  iprintf("invalid camera name!");
                  goto PARAM_ERROR;
               }
               set_cam_name(idx, it->second.c_str());
            }
            else if (str == "device_num") {
               if (!str2int(it->second, ival)) {
                  curr_it = it;
                  iprintf("invalid camera device num!");
                  goto PARAM_ERROR;
               }
               set_cam_dev_num(idx, ival);
            }
            else if (str == "pin_name") {
               if (it->second.empty()) {
                  curr_it = it;
                  iprintf("invalid pin name!");
                  goto PARAM_ERROR;
               }
               set_cam_pin_name(idx, it->second.c_str());
            }
            else if (str == "buff_size") {
               uint64_t lval;
               if (!str2byte(it->second, lval)) {
                  curr_it = it;
                  iprintf("invalid buffer size!");
                  goto PARAM_ERROR;
               }
               set_cam_buff_size(idx, lval);
            }
            else if (str == "frame_rate") {
               if (!str2double(it->second, rval)) {
                  curr_it = it;
                  iprintf("invalid frame rate!");
                  goto PARAM_ERROR;
               }
               set_cam_frame_rate(idx, rval);
            }
            else if (str == "pixel_format") {
               if (it->second.empty()) {
                  curr_it = it;
                  iprintf("invalid camera image format!");
                  goto PARAM_ERROR;
               }
               set_cam_fmt(idx, it->second.c_str());
            }
            else if (str == "pict_label") {
               if (it->second.empty()) {
                  curr_it = it;
                  iprintf("invalid camera image format!");
                  goto PARAM_ERROR;
               }
               set_cam_pict_label(idx, it->second.c_str());
            }
            else if (str == "spin_wheel") {
               if (it->second == "YES" || it->second == "yes"
                  || it->second == "y" || it->second == "1") {
                  set_cam_option(idx, TCameraDesc::kPict_Mark_Spin_wheel);
               }
               else if (it->second == "NO" || it->second == "no"
                  || it->second == "n" || it->second == "0") {
                  clr_cam_option(idx, TCameraDesc::kPict_Mark_Spin_wheel);
               }
               else {
                  iprintf("invalid camera image format!");
                  goto PARAM_ERROR;
               }
            }
            else if (str == "time_stamp") {
               if (it->second == "YES" || it->second == "yes"
                  || it->second == "y" || it->second == "1") {
                  set_cam_option(idx, TCameraDesc::kPict_Mark_Time_stamp);
               }
               else if (it->second == "NO" || it->second == "no"
                  || it->second == "n" || it->second == "0") {
                  clr_cam_option(idx, TCameraDesc::kPict_Mark_Time_stamp);
               }
               else {
                  iprintf("invalid camera image format!");
                  goto PARAM_ERROR;
               }
            }
            else {
               ++it;
               continue;
            }
         }
         else {
            ++it;
            continue;
         }

         param_list.erase(it);
         it = param_list.begin();
      }

      if (!param_list.empty()) {
         wprintf("the following parameter(s) ignored:");
         for (auto&& p : param_list) {
            fprintf(stderr, "> '%s' = '%s'\n", p.first.c_str(), p.second.c_str());
         }
         return false;
      }
      return true;

   PARAM_ERROR:
      eprintf("invalid parameter value: '%s' = '%s'!", (curr_it->first).c_str(), (curr_it->second).c_str());

   RETURN_ERROR:
      eprintf("read parameter from file '%s' failed!", fname);
      return false;
   };
}
