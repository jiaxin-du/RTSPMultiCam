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
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "libgroupsock.lib")
#pragma comment(lib, "libBasicUsageEnvironment.lib")
#pragma comment(lib, "libliveMedia.lib")
#pragma comment(lib, "libUsageEnvironment.lib")

#include "defines.h"
#include "GlobalParam.h"
#include "DataPacket.h"
#include "CameraCapture.h"
#include "CameraH264Encoder.h"
#include "CameraRTSPServer.h"

using namespace CameraStreaming;

CameraH264Encoder*  H264Encoder = nullptr;
CameraRTSPServer*   RTSPServer  = nullptr;
CameraCapture*      CamCapt[MAX_SUBPICT_COL * MAX_SUBPICT_ROW] = { nullptr };

int                THREAD_STATE = 0;
CRITICAL_SECTION   THREAD_STATE_MUTEX;

int                UDPPort;
int                HTTPTunnelPort;

DWORD WINAPI runRTSPServer(LPVOID RTSPServer)
{
   ((CameraRTSPServer *)RTSPServer)->run();
   EnterCriticalSection(&THREAD_STATE_MUTEX);
   THREAD_STATE = 1; //indicate a thread is terminated
   LeaveCriticalSection(&THREAD_STATE_MUTEX);
   iprintf("RTSP server thread finished!");
   ExitThread(NULL);
}

DWORD WINAPI runCapture(LPVOID cam)
{
   ((CameraCapture *)cam)->capture();
   EnterCriticalSection(&THREAD_STATE_MUTEX);
   THREAD_STATE = 1; //indicate a thread is terminated
   LeaveCriticalSection(&THREAD_STATE_MUTEX);
   iprintf("campturing thread finished!");
   ExitThread(NULL);
}

void CamOnFrame_0(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 0);
}

void CamOnFrame_1(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 1);
}

void CamOnFrame_2(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 2);
}

void CamOnFrame_3(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 3);
}

void CamOnFrame_4(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 4);
}

void CamOnFrame_5(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 5);
}

void CamOnFrame_6(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 6);
}

void CamOnFrame_7(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 7);
}

void CamOnFrame_8(DataPacket* buff)
{
   H264Encoder->receive_frame(buff, 8);
}

void cmd_usage(const char *cmd)
{
   printf("%s -l config.txt\n", cmd);
};

int main(int argc, const char* argv[])
{
   //
   av_register_all();
   av_register_all();
   avcodec_register_all();
   avdevice_register_all();
   av_log_set_level(AV_LOG_ERROR);

   int pict_w = 640;
   int pict_h = 480;

   int has_camera = 0;

   unsigned long bit_rate = 10 * 1024 * 1024;
   double        frame_rate = 15;

   char          fParamFileName[128] = R"(config.txt)";

   fParamFileName[127] = 0;

   for (int idx = 1; idx < argc; ++idx) {
      if (strcmp(argv[idx], "-l")) {
         ++idx;
         if (idx >= argc) {
            cmd_usage(argv[0]);
            return -1;
         }
         strncpy_s(fParamFileName, argv[idx], 127);
      }
   }

   iprintf("read configration from '%s'", fParamFileName);

   if (!GlobalParam::read_from_file(fParamFileName)) {
      wprintf("cannot find configuration file '%s'", fParamFileName);
      return -1;
   }

   //GlobalParam::print();

   //return -1;
   InitializeCriticalSectionAndSpinCount(&THREAD_STATE_MUTEX, 100);

   HANDLE cam_thread_id[MAX_SUBPICT_COL * MAX_SUBPICT_ROW] = { nullptr };
   HANDLE rtsp_thread_id = nullptr;

   H264Encoder = new CameraH264Encoder();
   if (H264Encoder == nullptr) {
      iprintf("allocating h264 encoder failed!");
      goto FINISHED;
   }

   H264Encoder->set_sub_pict(GlobalParam::sub_pict_width(), GlobalParam::sub_pict_height());
   H264Encoder->set_bit_rate(GlobalParam::bit_rate());
   H264Encoder->set_frame_rate(GlobalParam::frame_rate());
   H264Encoder->setup_encoder(AV_CODEC_ID_H264, GlobalParam::encoder_name());
   if (!H264Encoder->ready()) {
      iprintf("h264 encoder is not ready!");
      goto FINISHED;
   }

   for (int idx = 0; idx < GlobalParam::encoder_param_num(); ++idx) {
      H264Encoder->set_encoder_param(GlobalParam::encoder_param_name(idx),
         GlobalParam::encoder_param_val(idx));
   }

   for (int icam = 0; icam < GlobalParam::sub_pict_num(); ++icam) {
      cam_thread_id[icam] = nullptr;

      int icol = icam % GlobalParam::mosaic_col();
      int irow = icam / GlobalParam::mosaic_col();

      if (icol == 0) {
         GlobalParam::set_cam_option(icam, TCameraDesc::kPict_First_col);
      }
      else if (icol + 1 == GlobalParam::mosaic_col()) {
         GlobalParam::set_cam_option(icam, TCameraDesc::kPict_Last_col);
      }

      if (irow == 0) {
         GlobalParam::set_cam_option(icam, TCameraDesc::kPict_First_row);
      }
      else if (icol + 1 == GlobalParam::mosaic_row()) {
         GlobalParam::set_cam_option(icam, TCameraDesc::kPict_Last_row);
      }

      if (GlobalParam::cam_name(icam)[0] != 0) {
         iprintf("open camera '%s'(%d)", GlobalParam::cam_name(icam), GlobalParam::cam_dev_num(icam));

         CamCapt[icam] = new CameraCapture(GlobalParam::cam_desc(icam));
         if (CamCapt[icam] == nullptr) {
            eprintf("failed to create a object for camera '%s'(%d)!", GlobalParam::cam_name(icam), GlobalParam::cam_dev_num(icam));
            continue;
         }

         switch (icam)
         {
         case 0:
            CamCapt[icam]->set_callback_func(CamOnFrame_0);
            break;
         case 1:
            CamCapt[icam]->set_callback_func(CamOnFrame_1);
            break;
         case 2:
            CamCapt[icam]->set_callback_func(CamOnFrame_2);
            break;
         case 3:
            CamCapt[icam]->set_callback_func(CamOnFrame_3);
            break;
         case 4:
            CamCapt[icam]->set_callback_func(CamOnFrame_4);
            break;
         case 5:
            CamCapt[icam]->set_callback_func(CamOnFrame_5);
            break;
         case 6:
            CamCapt[icam]->set_callback_func(CamOnFrame_6);
            break;
         case 7:
            CamCapt[icam]->set_callback_func(CamOnFrame_7);
            break;
         case 8:
            CamCapt[icam]->set_callback_func(CamOnFrame_8);
            break;
         }
         
         CamCapt[icam]->initialise();

         if (!CamCapt[icam]->ready()) {
            eprintf("initialising camera '%s'(%d) failed!", GlobalParam::cam_name(icam), GlobalParam::cam_dev_num(icam));
            delete CamCapt[icam];
            CamCapt[icam] = nullptr;
         }
         else {
            ++has_camera;
         }
      }
      else {
         CamCapt[icam] = nullptr;
      }
   }

   if (has_camera == 0) {
      eprintf("no camera is opened!");
      goto FINISHED;
   }


   RTSPServer = new CameraRTSPServer(H264Encoder, 8554, 8080);

   for (int icam = 0; icam < GlobalParam::sub_pict_num(); ++icam) {
      if (CamCapt[icam] != nullptr) {
         cam_thread_id[icam] = CreateThread(nullptr, // default security attributes
            0,                                    // default stack size
            (LPTHREAD_START_ROUTINE)runCapture,   //the routine
            CamCapt[icam],                        //function argument
            0,                                    //default creation flags
            nullptr);

         if (cam_thread_id[icam] == nullptr) {
            //exception
            eprintf("creating thread for capturing '%s' failed!", GlobalParam::cam_name(icam));
            goto FINISHED;
         }
      }
      else {
         cam_thread_id[icam] = nullptr;
      }
   }

   rtsp_thread_id = CreateThread(nullptr, // default security attributes
      0,                               // default stack size
      (LPTHREAD_START_ROUTINE)runRTSPServer, //the routine
      RTSPServer,                      //function argument
      0,                               //default creation flags
      nullptr);

   if (rtsp_thread_id == nullptr) {
      //exception
      eprintf("creating thread for RTSP server failed!");
      goto FINISHED;
   }

   // Play Media Here
   H264Encoder->run();

   EnterCriticalSection(&THREAD_STATE_MUTEX);
   THREAD_STATE = 1; //indicate a thread is terminated
   LeaveCriticalSection(&THREAD_STATE_MUTEX);
   iprintf("encoding thread finished!");

FINISHED:
   if (RTSPServer != nullptr) {
      RTSPServer->signal_exit();
   }

   if (H264Encoder != nullptr) {
      delete H264Encoder;
   }

   Sleep(1000);  //wait for the thread to exit;

   DWORD exit_code;
   if (RTSPServer != nullptr) {
      if (rtsp_thread_id != nullptr && GetExitCodeThread(rtsp_thread_id, &exit_code) && exit_code == STILL_ACTIVE) {
         wprintf("RTSP server thread still running, kill it!");
         TerminateThread(rtsp_thread_id, -1);
      }
      delete RTSPServer;
   }

   for (int icam = 0; icam < GlobalParam::sub_pict_num(); ++icam) {
      if (CamCapt[icam] != nullptr) {
         if (cam_thread_id[icam] != nullptr && GetExitCodeThread(cam_thread_id[icam], &exit_code) && exit_code == STILL_ACTIVE) {
            wprintf("thread for camera '%s' still running, kill it!", (GlobalParam::cam_desc(icam)->name).c_str());
            TerminateThread(cam_thread_id[icam], -1);
         }
         delete CamCapt[icam];
      }
   }

   DeleteCriticalSection(&THREAD_STATE_MUTEX);
}
