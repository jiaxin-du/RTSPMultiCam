Skip to content
Features
Business
Explore
Marketplace
Pricing
This repository
Search
Sign in or Sign up
2 5 2 argvk/ffmpeg-examples
 Code  Issues 0  Pull requests 0  Projects 0  Insights
ffmpeg-examples/dshow_capture_video.c
4c8b30e  on May 24, 2014
@argvk argvk add newer example using newer api
     
412 lines (324 sloc)  12.3 KB
/**
 * captures from dshow input and writes to out.webm
 * captures just the video
 *
 * compile using
 * > gcc dshow_capture_video.c -I"..\ffmpeg-dev\include" -L"..\ffmpeg-shared" -o dshow_capture_video -lavcodec-55 -lavdevice-55 -lavfilter-4 -lavformat-55 -lavutil-52 -Wall
 */
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

int main(int argc, char **argv)
{
  // register all codecs/devices/filters
  av_register_all(); 
  avcodec_register_all();
  avdevice_register_all();
  avfilter_register_all();

  // setup the codec & codec context
  AVCodec *pCodecOut;
  AVCodec *pCodecInCam;

  AVCodecContext *pCodecCtxOut= NULL;
  AVCodecContext *pCodecCtxInCam = NULL;

  char args_cam[512]; // we'll use this in the filter_graph

  int i, ret, got_output,video_stream_idx_cam;

  // the frames to be used
  AVFrame *cam_frame,*outFrame,*filt_frame;
  AVPacket packet;

  // output filename
  const char *filename = "out.webm";

  AVOutputFormat *pOfmtOut = NULL;
  AVStream *strmVdoOut = NULL;

  // we will capture from a dshow device
  // @see https://trac.ffmpeg.org/wiki/DirectShow
  // @see http://www.ffmpeg.org/ffmpeg-devices.html#dshow
  AVInputFormat *inFrmt= av_find_input_format("dshow");

  // used to set the input options
  AVDictionary *inOptions = NULL;

  // set the format context
  AVFormatContext *pFormatCtxOut;
  AVFormatContext *pFormatCtxInCam;

  // to create the filter graph
  AVFilter *buffersrc_cam  = avfilter_get_by_name("buffer");
  AVFilter *buffersink = avfilter_get_by_name("buffersink");

  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs  = avfilter_inout_alloc();

  AVFilterContext *buffersink_ctx;
  AVFilterContext *buffersrc_ctx_cam;
  AVFilterGraph *filter_graph;

  AVBufferSinkParams *buffersink_params;

  // the filtergraph, string, scale keeping the height constant, and setting the pixel format
  const char *filter_str = "scale='w=-1:h=480',format='yuv420p'"; // add padding for 16:9 and then scale to 1280x720

  /////////////////////////// decoder 

  // create the new context
  pFormatCtxInCam = avformat_alloc_context();

  // set input resolution
  av_dict_set(&inOptions, "video_size", "640x480", 0);
  av_dict_set(&inOptions, "r", "25", 0);

  // input device, since we selected dshow
  ret = avformat_open_input(&pFormatCtxInCam, "video=ManyCam Virtual Webcam", inFrmt, &inOptions);

  // lookup infor
  if(avformat_find_stream_info(pFormatCtxInCam,NULL)<0)
    return -1;

  video_stream_idx_cam = -1;

  // find the stream index, we'll only be encoding the video for now
  for(i=0; i<pFormatCtxInCam->nb_streams; i++)
    if(pFormatCtxInCam->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
      video_stream_idx_cam=i;

  if(video_stream_idx_cam == -1)
    return -1; 

  // set the coded ctx & codec
  pCodecCtxInCam = pFormatCtxInCam->streams[video_stream_idx_cam]->codec;

  pCodecInCam = avcodec_find_decoder(pCodecCtxInCam->codec_id);

  if(pCodecInCam == NULL){
    fprintf(stderr,"decoder not fiund");
    exit(1);
  }

  // open it
  if (avcodec_open2(pCodecCtxInCam, pCodecInCam, NULL) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder for webcam\n");
    exit(1);
  }

  //////////////////////// encoder

  // find the format based on the filename
  pOfmtOut = av_guess_format(NULL, filename,NULL);

  pFormatCtxOut = avformat_alloc_context();

  // set the codec
  pOfmtOut->video_codec = AV_CODEC_ID_VP8;

  // set the output format
  pFormatCtxOut->oformat = pOfmtOut;  

  snprintf(pFormatCtxOut->filename,sizeof(pFormatCtxOut->filename),"%s",filename);

  // add a new video stream
  strmVdoOut = avformat_new_stream(pFormatCtxOut,NULL);
  if (!strmVdoOut ) {
    fprintf(stderr, "Could not alloc stream\n");
    exit(1);
  }

  pCodecCtxOut = strmVdoOut->codec;
  if (!pCodecCtxOut) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  // set the output codec params
  pCodecCtxOut->codec_id = pOfmtOut->video_codec;
  pCodecCtxOut->codec_type = AVMEDIA_TYPE_VIDEO;

  pCodecCtxOut->bit_rate = 1200000;
  pCodecCtxOut->pix_fmt = AV_PIX_FMT_YUV420P;
  pCodecCtxOut->width = 640;
  pCodecCtxOut->height = 480;
  pCodecCtxOut->time_base = (AVRational){1,25};

  // if the format wants global header, we'll set one
  if(pFormatCtxOut->oformat->flags & AVFMT_GLOBALHEADER)
    pCodecCtxOut->flags |= CODEC_FLAG_GLOBAL_HEADER;

  // find the encoder
  pCodecOut = avcodec_find_encoder(pCodecCtxOut->codec_id);
  if (!pCodecOut) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }

  // and open it
  if (avcodec_open2(pCodecCtxOut, pCodecOut,NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  // open the file for writing
  if (avio_open(&pFormatCtxOut->pb, filename, AVIO_FLAG_WRITE) <0) {
    fprintf(stderr, "Could not open '%s'\n", filename);
    exit(1);
  }

  // write the format headers
  ret = avformat_write_header(pFormatCtxOut, NULL);
  if(ret < 0 ){
    fprintf(stderr, "Could not write header '%d'\n", ret);
    exit(1);
  }

  ////////////////// create frame 

  // allocate frames
  // these can be done down under also
  cam_frame = av_frame_alloc();
  outFrame = av_frame_alloc();
  filt_frame = av_frame_alloc();

  if (!cam_frame || !outFrame || !filt_frame) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }

  ////////////////////////// fix pix fmt
  enum AVPixelFormat pix_fmts[] = { pCodecCtxInCam->pix_fmt,  AV_PIX_FMT_NONE};

  /////////////////////////// FILTER GRAPH

  // create a filter graph
  filter_graph = avfilter_graph_alloc();

  snprintf(args_cam, sizeof(args_cam),
      "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      pCodecCtxInCam->width, pCodecCtxInCam->height, pCodecCtxInCam->pix_fmt,
      pCodecCtxInCam->time_base.num, pCodecCtxInCam->time_base.den,
      pCodecCtxInCam->sample_aspect_ratio.num, pCodecCtxInCam->sample_aspect_ratio.den);

  // input source buffer
  ret = avfilter_graph_create_filter(&buffersrc_ctx_cam, buffersrc_cam, "in",
      args_cam, NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
    exit(ret);
  }

  buffersink_params = av_buffersink_params_alloc();
  buffersink_params->pixel_fmts = pix_fmts;

  // output sink
  ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
      NULL, buffersink_params, filter_graph);
  av_free(buffersink_params);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
    exit(ret);
  }

  outputs->name       = av_strdup("in");
  outputs->filter_ctx = buffersrc_ctx_cam;
  outputs->pad_idx    = 0;
  outputs->next       = NULL;

  inputs->name       = av_strdup("out");
  inputs->filter_ctx = buffersink_ctx;
  inputs->pad_idx    = 0;
  inputs->next       = NULL;

  // parse the filter string we set
  if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_str,
          &inputs, &outputs, NULL)) < 0){
    printf("error in graph parse");
    exit(ret);
  }

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0){
    printf("error in graph config");
    exit(ret);
  }
  //////////////////////////////DUMP
 
  // for info stuff
  av_dump_format(pFormatCtxInCam,0,"cam",0);
  av_dump_format(pFormatCtxOut,0,filename,1);

  ////////////////////////// TRANSCODE

  // since a dshow is never ending, i've just a static number
  for(i=0;i<300;i++){
    av_init_packet(&packet);

    // get a frame from input
    ret = av_read_frame(pFormatCtxInCam,&packet);

    if(ret < 0){
      fprintf(stderr,"Error reading frame\n");
      break;
    }

    // chaeck if its a avideo frame
    if(packet.stream_index == video_stream_idx_cam){

      // set the dts & pts
      // @see http://stackoverflow.com/q/6044330/651547
      packet.dts = av_rescale_q_rnd(packet.dts,
          pFormatCtxInCam->streams[video_stream_idx_cam]->time_base,
          pFormatCtxInCam->streams[video_stream_idx_cam]->codec->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
      packet.pts = av_rescale_q_rnd(packet.pts,
          pFormatCtxInCam->streams[video_stream_idx_cam]->time_base,
          pFormatCtxInCam->streams[video_stream_idx_cam]->codec->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      // decode from the format and store in cam_frame
      ret = avcodec_decode_video2(pCodecCtxInCam,cam_frame,&got_output,&packet);

      cam_frame->pts = av_frame_get_best_effort_timestamp(cam_frame);

      if (got_output) {

        av_free_packet(&packet);
        av_init_packet(&packet);

        // add frame to filter graph 
        ret = av_buffersrc_add_frame_flags(buffersrc_ctx_cam, cam_frame, 0);

        // get the frames from the filter graph
        while(1){
          filt_frame = av_frame_alloc();
          if ( ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
            break;
          }

          // we'll get a frame that we can now encode to
          ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
          if (ret < 0) {
            // if there are no more frames or end of frames that is normal
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
              ret = 0;
            av_frame_free(&filt_frame);
            break;
          }

          // this is redundant
          outFrame = filt_frame;

          // encode according the output format
          ret = avcodec_encode_video2(pCodecCtxOut, &packet, outFrame, &got_output);
          if (got_output) {

            // dts, pts dance
            packet.dts = av_rescale_q_rnd(packet.dts,
                pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
                pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

            packet.pts = av_rescale_q_rnd(packet.pts,
                pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
                pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

            packet.duration = av_rescale_q(packet.duration,
                pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
                pFormatCtxOut->streams[video_stream_idx_cam]->time_base
                );

            // and write it to file
            if(av_interleaved_write_frame(pFormatCtxOut,&packet) < 0){
              fprintf(stderr,"error writing frame");
              exit(1);
            }
          }
          if (ret < 0){
            av_frame_free(&filt_frame);
            break;
          }
        }
      }
    }
    // free the packet everytime
    av_free_packet(&packet);
  }

  // flush out delayed frames
  for (got_output = 1; got_output; i++) {
    ret = avcodec_encode_video2(pCodecCtxOut, &packet, NULL, &got_output);
    if (ret < 0) {
      fprintf(stderr, "Error encoding frame\n");
      exit(1);
    }

    if (got_output) {
      packet.dts = av_rescale_q_rnd(packet.dts,
          pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
          pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      packet.pts = av_rescale_q_rnd(packet.pts,
          pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
          pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      packet.duration = av_rescale_q(packet.duration,
          pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
          pFormatCtxOut->streams[video_stream_idx_cam]->time_base
          );

      if(pCodecCtxOut->coded_frame->key_frame)
        packet.flags |= AV_PKT_FLAG_KEY;

      ret = av_interleaved_write_frame(pFormatCtxOut,&packet);
      if(ret < 0 ){
        exit(1);
      }       
      av_free_packet(&packet);
    }
  }

  ret = av_write_trailer(pFormatCtxOut);
  if(ret < 0 ){
    exit(1);   
  }

  /////////////// close everything
  avcodec_close(strmVdoOut->codec);
  avcodec_close(pCodecCtxInCam);
  avcodec_close(pCodecCtxOut);
  for(i = 0; i < pFormatCtxOut->nb_streams; i++) {
    av_freep(&pFormatCtxOut->streams[i]->codec);
    av_freep(&pFormatCtxOut->streams[i]);
  }
  avio_close(pFormatCtxOut->pb);
  av_free(pFormatCtxOut);
  av_free(pFormatCtxInCam);
  av_dict_free(&inOptions);

  return 0;
}

© 2018 GitHub, Inc.
Terms
Privacy
Security
Status
Help
Contact GitHub
API
Training
Shop
Blog
About
Press h to open a hovercard with more details.