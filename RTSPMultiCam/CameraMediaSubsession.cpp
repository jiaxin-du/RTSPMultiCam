//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#include "CameraMediaSubsession.h"

namespace CameraStreaming
{
   CameraMediaSubsession* CameraMediaSubsession::createNew(UsageEnvironment& env, StreamReplicator* replicator)
   {
      return new CameraMediaSubsession(env, replicator);
   }

   FramedSource* CameraMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
   {
      estBitrate = fBitRate;
      FramedSource* source = m_replicator->createStreamReplica();
      return H264VideoStreamDiscreteFramer::createNew(envir(), source);
   }

   RTPSink* CameraMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
   {
      return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, fSPSNAL, fSPSNALSize, fPPSNAL, fPPSNALSize);
   }
}
