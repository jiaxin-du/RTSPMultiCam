//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma  once

#include "misc.h"
#include "FramedSource.hh"
#include "UsageEnvironment.hh"
#include "Groupsock.hh"
#include "GroupsockHelper.hh"
#include "CameraH264Encoder.h"

namespace CameraStreaming
{
   class CameraFrameSource : public FramedSource {
   public:
      static CameraFrameSource* createNew(UsageEnvironment&, CameraH264Encoder*);
      CameraFrameSource(UsageEnvironment& env, CameraH264Encoder*);
      ~CameraFrameSource() = default;

   private:
      static void deliverFrameStub(void* clientData) {
         ((CameraFrameSource*)clientData)->deliverFrame();
      };
      void doGetNextFrame() override;
      void deliverFrame();
      void doStopGettingFrames() override;
      void onFrame();

   private:
      CameraH264Encoder   *fEncoder;
      EventTriggerId       fEventTriggerId;
   };
}

