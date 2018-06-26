//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#include "misc.h"
#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"
#include "CameraMediaSubsession.h"
#include "CameraFrameSource.h"
#include "CameraH264Encoder.h"

namespace CameraStreaming
{
   class CameraRTSPServer
   {
   public:

      CameraRTSPServer(CameraH264Encoder  * enc, int port, int httpPort);
      ~CameraRTSPServer();
     
      void run();

      void signal_exit() {
         fQuit = 1;
      };

      void set_bit_rate(uint64_t br) {
         if (br < 102400) {
            fBitRate = 100;
         }
         else {
            fBitRate = static_cast<unsigned int>(br / 1024); //in kbs
         }
      };

   private:
      int                 fPortNum;
      int                 fHttpTunnelingPort;
      unsigned int        fBitRate;
      CameraH264Encoder  *fH264Encoder;
      char                fQuit;
   };
}
