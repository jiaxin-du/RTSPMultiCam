//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#pragma warning(disable : 4996)

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "strmiids.lib")
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include <Windows.h>
#include <mmdeviceapi.h>
#include <uuids.h>
#include <conio.h>
#include <dshow.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

using std::string;

void show_devices()
{
   AVFormatContext *pFormatCtx = avformat_alloc_context();
   AVDictionary* options = NULL;
   av_dict_set(&options, "list_devices", "true", 0);
   AVInputFormat *iformat = av_find_input_format("dshow");
   //print_green_text(stdout, "[Info]");
   av_log(NULL, AV_LOG_INFO, "----- available cameras -----\n");
   avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
   av_log(NULL, AV_LOG_INFO, "----- list end -----\n");
   avformat_close_input(&pFormatCtx);
}

void show_device_options(const string& cam, int device_num = 0)
{
   string src = "video=";
   src += cam;
   AVFormatContext *pFormatCtx = avformat_alloc_context();
   AVDictionary* options = NULL;
   av_dict_set(&options, "list_options", "true", 0);
   av_dict_set(&options, "video_device_number", std::to_string(device_num).c_str(), 0);
   AVInputFormat *iformat = av_find_input_format("dshow");
   //print_green_text(stdout, "[Info]");
   av_log(NULL, AV_LOG_INFO, "----- available format -----\n  device name: '%s'\n  device_num: %d\n", cam.c_str(), device_num);
   avformat_open_input(&pFormatCtx, src.c_str(), iformat, &options);
   av_log(NULL, AV_LOG_INFO, "----- list end -----\n");
}

int main(int argc, char **argv)
{
   // register all codecs/devices/filters
   av_register_all();
   avcodec_register_all();
   avdevice_register_all();
   avfilter_register_all();

   int ret;

   show_devices();

   std::vector<string> device_name;

   CoInitialize(NULL);

   ICreateDevEnum *pDevEnum = NULL;
   HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, 0, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
   if (FAILED(hr)) {
      av_log(NULL, AV_LOG_ERROR, "failed to create a dshow instance! err = 0x%08x\n", hr);
      return -1;
   }
   IEnumMoniker *pEnum;
   hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
   if (FAILED(hr)) {
      av_log(NULL, AV_LOG_ERROR, "failed to get a list of dshow input devices!");
      return -1;
   }
   IMoniker *pMoniker = NULL;
   while (S_OK == pEnum->Next(1, &pMoniker, NULL)) {
      IPropertyBag *pPropBag;
      LPOLESTR str = 0;
      hr = pMoniker->GetDisplayName(0, 0, &str);
      if (SUCCEEDED(hr)) {
         hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
         if (SUCCEEDED(hr)) {
            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
            std::wstring fName = var.bstrVal;
            device_name.push_back(string(fName.begin(), fName.end()));
         }
         else {
            av_log(NULL, AV_LOG_ERROR, "could not bind device property to storage");
         }
      }
   }

   std::sort(device_name.begin(), device_name.end());
   int device_num = 0;
   string pre_dev = "";
   for (auto&& dev : device_name) {
      if (!dev.empty()) {
         if (dev == pre_dev) {
            ++device_num;
         }
         else {
            device_num = 0;
            pre_dev = dev;
         }
         show_device_options(dev.c_str(), device_num);
      }
   }

   string fCamName;
   for (auto&& dev : device_name) {
      if (!dev.empty()) {
         fCamName = dev;
         break;
      }
   }

   if (fCamName.empty()) {
      printf("no webcam can be used!");
      return 0;
   }

   string video_src = "video=";
   video_src += fCamName;
   AVFormatContext *pFormatCtx = avformat_alloc_context();
   AVDictionary* options = NULL;

   av_dict_set(&options, "video_device_number", std::to_string(device_num).c_str(), 0);
   av_dict_set(&options, "video_size", "640x480", 0);
   av_dict_set(&options, "framerate", "30", 0);

   AVInputFormat *iformat = av_find_input_format("dshow");
   /*
  char* str_tmp = NULL;
  av_dict_get_string(options, &str_tmp, '=', ';');

  av_log(options, AV_LOG_INFO, "open '%s' with options '%s'", video_src.c_str(), str_tmp);
  //free(str_tmp);
     */
   ret = avformat_open_input(&pFormatCtx, video_src.c_str(), iformat, &options);
   if (ret < 0) {
      av_log(pFormatCtx, AV_LOG_ERROR, "open input failed!\n");
   }

   av_log(options, AV_LOG_INFO, "test source '%s'.\n", fCamName.c_str());
   //print out the video stream information
   printf("video stream from '%s': ", fCamName.c_str());
   av_dump_format(pFormatCtx, 0, video_src.c_str(), 0);

   int ost_num = -1;
   AVStream*  stream = NULL;
   for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
      if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
         ost_num = i;
         stream = pFormatCtx->streams[i];
      }
   }

   if (ost_num == -1 || stream == NULL)
   {
      //exception
      av_log(pFormatCtx, AV_LOG_ERROR, "no video stream found in '%s'!\n", fCamName.c_str());
      return -1;
   }
   std::clock_t tm_1, tm_2, tm_diff;
   std::clock_t tot_tm = 0, min_tm = 1000, max_tm = 0;
   AVPacket * packet = av_packet_alloc();

   std::map<int, int> delay_cnt;
   int delay_msec;
   for (int ipkt = 0; ipkt < 1000; ++ipkt) {
      tm_1 = std::clock();
      ret = av_read_frame(pFormatCtx, packet);
      tm_2 = std::clock();
      if (ret < 0) {
         break;
      }
      tm_diff = tm_2 - tm_1;
      delay_msec = static_cast<int>(1000.* (double)tm_diff / (double)CLOCKS_PER_SEC + 0.5);
      ++delay_cnt[delay_msec];
      max_tm = max(max_tm, tm_diff);
      min_tm = min(min_tm, tm_diff);
      tot_tm += tm_diff;
      av_free_packet(packet);
   }

   av_log(NULL, AV_LOG_INFO, "time for reading a frame: avg = %g (%g-%g) msec", tot_tm / (double)CLOCKS_PER_SEC,
      1000. * min_tm / (double)CLOCKS_PER_SEC,
      1000. * max_tm / (double)CLOCKS_PER_SEC);

   printf("delay count in msec\n");
   for (auto&& p : delay_cnt) {
      printf("   %5d: %5d\n", p.first, p.second);
   }

   return 0;
}