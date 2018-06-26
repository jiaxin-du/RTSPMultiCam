//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#include "CameraRTSPServer.h"

extern int THREAD_STATE;
extern CRITICAL_SECTION   THREAD_STATE_MUTEX;

namespace CameraStreaming
{
   CameraRTSPServer::CameraRTSPServer(CameraH264Encoder * enc, int port, int httpPort)
      : fH264Encoder(enc), fPortNum(port), fHttpTunnelingPort(httpPort), fQuit(0), fBitRate(5120) //in kbs
   {
      fQuit = 0;
   };

   CameraRTSPServer::~CameraRTSPServer()
   {
   };

   void CameraRTSPServer::run()
   {
      TaskScheduler    *scheduler;
      UsageEnvironment *env;
      char stream_name[] = "MultiCam";

      scheduler = BasicTaskScheduler::createNew();
      env = BasicUsageEnvironment::createNew(*scheduler);

      UserAuthenticationDatabase* authDB = nullptr;

      // if (m_Enable_Pass){
      // 	authDB = new UserAuthenticationDatabase;
      // 	authDB->addUserRecord(UserN, PassW);
      // }

      OutPacketBuffer::increaseMaxSizeTo(5242880);  // 1M
      RTSPServer* rtspServer = RTSPServer::createNew(*env, fPortNum, authDB);

      if (rtspServer == nullptr)
      {
         *env << "LIVE555: Failed to create RTSP server: %s\n", env->getResultMsg();
      }
      else {
         if (fHttpTunnelingPort)
         {
            rtspServer->setUpTunnelingOverHTTP(fHttpTunnelingPort);
         }

         char const* descriptionString = "Combined Multiple Cameras Streaming Session";

         CameraFrameSource* source = CameraFrameSource::createNew(*env, fH264Encoder);
         StreamReplicator* inputDevice = StreamReplicator::createNew(*env, source, false);

         ServerMediaSession* sms = ServerMediaSession::createNew(*env, stream_name, stream_name, descriptionString); 
         CameraMediaSubsession* sub = CameraMediaSubsession::createNew(*env, inputDevice);

         sub->set_PPS_NAL(fH264Encoder->PPS_NAL(), fH264Encoder->PPS_NAL_size());
         sub->set_SPS_NAL(fH264Encoder->SPS_NAL(), fH264Encoder->SPS_NAL_size());
         if (fH264Encoder->h264_bit_rate() > 102400) {
            sub->set_bit_rate(fH264Encoder->h264_bit_rate());
         }
         else {
            sub->set_bit_rate(static_cast<uint64_t>(fH264Encoder->pict_width() * fH264Encoder->pict_height() * fH264Encoder->frame_rate() / 20));
         }
         
         sms->addSubsession(sub);
         rtspServer->addServerMediaSession(sms);

         char* url = rtspServer->rtspURL(sms);
         *env << "Play this stream using the URL \"" << url << "\"\n";
         delete[] url;

         //signal(SIGNIT,sighandler);
         env->taskScheduler().doEventLoop(&fQuit); // does not return

         Medium::close(rtspServer);
         Medium::close(inputDevice);
      }

      env->reclaim();
      delete scheduler;
   };
}
