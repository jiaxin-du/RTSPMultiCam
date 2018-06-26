//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#pragma once

#include "misc.h"
#include "OnDemandServerMediaSubsession.hh"
#include "StreamReplicator.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoStreamDiscreteFramer.hh"
#include "UsageEnvironment.hh"
#include "Groupsock.hh"

namespace CameraStreaming
{
   class CameraMediaSubsession : public OnDemandServerMediaSubsession
   {
   public:
      static CameraMediaSubsession* createNew(UsageEnvironment& env, StreamReplicator* replicator);

      void set_PPS_NAL(const uint8_t* ptr, int size) {
         fPPSNAL = ptr;
         fPPSNALSize = size;
      };

      void set_SPS_NAL(const uint8_t* ptr, int size) {
         fSPSNAL = ptr;
         fSPSNALSize = size;
      };

      void set_bit_rate(uint64_t br) {
         if (br > 102400) {
            fBitRate = static_cast<unsigned long>(br / 1024);
         }
         else {
            wprintf("estimated bit rate %lld too small, set to 100K!", br);
            fBitRate = 100;
         }
      };
     
   protected:
      CameraMediaSubsession(UsageEnvironment& env, StreamReplicator* replicator)
         : OnDemandServerMediaSubsession(env, True), m_replicator(replicator), fBitRate(1024) , fPPSNAL(nullptr), fPPSNALSize(0),
         fSPSNAL(nullptr), fSPSNALSize(0)
      {
      };

      FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate) override;
      RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) override;

      StreamReplicator   *m_replicator;
      unsigned long       fBitRate;
      const uint8_t*      fPPSNAL;
      int                 fPPSNALSize;
      const uint8_t*      fSPSNAL;
      int                 fSPSNALSize;
   };
}
