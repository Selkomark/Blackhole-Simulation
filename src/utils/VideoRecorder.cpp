#include "../../include/utils/VideoRecorder.hpp"
#include <iostream>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>

// FFmpeg headers - wrap in extern "C" to prevent C++ name mangling
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
}

struct FFmpegContext {
  AVFormatContext* formatContext;
  AVCodecContext* codecContext;
  AVStream* videoStream;
  AVFrame* frame;
  AVPacket* packet;
  SwsContext* swsContext;
  int frameCount;
};

VideoRecorder::VideoRecorder()
    : recording(false), frameWidth(0), frameHeight(0), frameRate(60), ffmpegContext(nullptr) {
}

VideoRecorder::~VideoRecorder() {
  stopRecording();
}

bool VideoRecorder::startRecording(const std::string& file, int width, int height, int fps) {
  if (recording) {
    std::cerr << "Already recording!" << std::endl;
    return false;
  }
  
  filename = file;
  frameWidth = width;
  frameHeight = height;
  frameRate = fps;
  
  // Generate filename with timestamp if not provided
  if (filename.empty()) {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << "blackhole_recording_" 
        << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".mp4";
    filename = oss.str();
  }
  
  // Initialize encoder first, only set recording flag if successful
  if (initializeEncoder()) {
    recording = true;
    return true;
  } else {
    recording = false;
    return false;
  }
}

bool VideoRecorder::initializeEncoder() {
  FFmpegContext* ctx = new FFmpegContext();
  ctx->formatContext = nullptr;
  ctx->codecContext = nullptr;
  ctx->videoStream = nullptr;
  ctx->frame = nullptr;
  ctx->packet = nullptr;
  ctx->swsContext = nullptr;
  ctx->frameCount = 0;
  ffmpegContext = ctx;
  
  // Allocate format context
  int ret = avformat_alloc_output_context2(&ctx->formatContext, nullptr, nullptr, filename.c_str());
  if (ret < 0 || !ctx->formatContext) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    std::cerr << "Could not create output context: " << errbuf << std::endl;
    cleanupEncoder();
    return false;
  }
  
  // Find H.264 encoder - prefer libx264 software encoder (more reliable than VideoToolbox)
  // Try to find libx264 explicitly first
  const AVCodec* codec = nullptr;
  
  // List all available H.264 encoders to find libx264
  void* iter = nullptr;
  while (true) {
    const AVCodec* c = av_codec_iterate(&iter);
    if (!c) break;
    if (c->id == AV_CODEC_ID_H264 && av_codec_is_encoder(c)) {
      if (strcmp(c->name, "libx264") == 0) {
        codec = c;
        break;
      }
    }
  }
  
  // If libx264 not found, try VideoToolbox but configure it carefully
  if (!codec) {
    codec = avcodec_find_encoder_by_name("h264_videotoolbox");
    if (!codec) {
      codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (codec && strcmp(codec->name, "h264_videotoolbox") == 0) {
      std::cerr << "Warning: libx264 not available, using VideoToolbox (may have limitations)" << std::endl;
    }
  }
  
  if (!codec) {
    std::cerr << "H.264 codec not found" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  std::cout << "Using encoder: " << codec->name << std::endl;
  
  // Create codec context
  ctx->codecContext = avcodec_alloc_context3(codec);
  if (!ctx->codecContext) {
    std::cerr << "Could not allocate codec context" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  // Set codec parameters
  ctx->codecContext->codec_id = codec->id;
  ctx->codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
  ctx->codecContext->width = frameWidth;
  ctx->codecContext->height = frameHeight;
  ctx->codecContext->time_base = {1, frameRate};
  ctx->codecContext->framerate = {frameRate, 1};
  ctx->codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
  
  // Set quality preset (only for libx264, VideoToolbox uses different options)
  bool isVideoToolbox = (strcmp(codec->name, "h264_videotoolbox") == 0);
  
  if (strcmp(codec->name, "libx264") == 0) {
    // libx264 supports preset and crf
    ctx->codecContext->gop_size = 10;
    ctx->codecContext->max_b_frames = 1;
    if (av_opt_set(ctx->codecContext->priv_data, "preset", "medium", 0) < 0) {
      std::cerr << "Warning: Could not set preset" << std::endl;
    }
    if (av_opt_set(ctx->codecContext->priv_data, "crf", "23", 0) < 0) {
      std::cerr << "Warning: Could not set CRF" << std::endl;
    }
  } else if (isVideoToolbox) {
    // VideoToolbox configuration - the bitrate error occurs because FFmpeg tries to
    // set bitrate properties internally. We need to work around this.
    
    // Set basic codec parameters
    ctx->codecContext->gop_size = 0; // Let VideoToolbox decide
    ctx->codecContext->max_b_frames = 0; // VideoToolbox doesn't support B-frames well
    
    // Set profile to baseline to avoid advanced features
    ctx->codecContext->profile = FF_PROFILE_H264_BASELINE;
    ctx->codecContext->level = 40;
    
    // Try setting a reasonable bitrate to avoid the error
    // VideoToolbox seems to require bitrate to be set, so set it explicitly
    // Calculate bitrate based on resolution and frame rate for good quality
    // Higher resolution needs more bitrate: ~8-10 Mbps for 1080p, ~20 Mbps for 4K
    int pixelsPerFrame = frameWidth * frameHeight;
    int targetBitrate;
    if (pixelsPerFrame >= 3840 * 2160) {
      // 4K: ~20 Mbps
      targetBitrate = 20000000;
    } else if (pixelsPerFrame >= 2560 * 1440) {
      // 1440p: ~12 Mbps
      targetBitrate = 12000000;
    } else if (pixelsPerFrame >= 1920 * 1080) {
      // 1080p: ~8 Mbps
      targetBitrate = 8000000;
    } else if (pixelsPerFrame >= 1280 * 720) {
      // 720p: ~4 Mbps
      targetBitrate = 4000000;
    } else {
      // Lower resolutions: ~2 Mbps
      targetBitrate = 2000000;
    }
    
    // Adjust for frame rate (higher FPS needs more bitrate)
    targetBitrate = (targetBitrate * frameRate) / 30; // Normalize to 30fps baseline
    
    ctx->codecContext->bit_rate = targetBitrate;
    ctx->codecContext->rc_max_rate = targetBitrate;
    ctx->codecContext->rc_min_rate = targetBitrate / 2;
    ctx->codecContext->rc_buffer_size = targetBitrate;
    
    // Set VideoToolbox-specific options
    av_opt_set(ctx->codecContext->priv_data, "allow-frame-reordering", "0", 0);
    av_opt_set(ctx->codecContext->priv_data, "realtime", "1", 0);
    
    std::cout << "VideoToolbox: Using bitrate " << (targetBitrate / 1000000) << " Mbps for " 
              << frameWidth << "×" << frameHeight << "@" << frameRate << "fps" << std::endl;
  } else {
    // For other encoders, try generic options
    ctx->codecContext->gop_size = 10;
    ctx->codecContext->max_b_frames = 1;
    av_opt_set(ctx->codecContext->priv_data, "crf", "23", 0);
  }
  
  // Open codec
  ret = avcodec_open2(ctx->codecContext, codec, nullptr);
  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    std::cerr << "Could not open codec: " << errbuf << std::endl;
    cleanupEncoder();
    return false;
  }
  
  // Create video stream
  ctx->videoStream = avformat_new_stream(ctx->formatContext, codec);
  if (!ctx->videoStream) {
    std::cerr << "Could not create video stream" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  ctx->videoStream->id = ctx->formatContext->nb_streams - 1;
  ctx->videoStream->codecpar->codec_id = codec->id;
  ctx->videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
  ctx->videoStream->codecpar->width = frameWidth;
  ctx->videoStream->codecpar->height = frameHeight;
  ctx->videoStream->time_base = {1, frameRate};
  avcodec_parameters_from_context(ctx->videoStream->codecpar, ctx->codecContext);
  
  // Open output file
  if (!(ctx->formatContext->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&ctx->formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
      std::cerr << "Could not open output file " << filename << ": " << errbuf << std::endl;
      cleanupEncoder();
      return false;
    }
  }
  
  // Write header
  ret = avformat_write_header(ctx->formatContext, nullptr);
  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    std::cerr << "Could not write header: " << errbuf << std::endl;
    cleanupEncoder();
    return false;
  }
  
  // Allocate frame
  ctx->frame = av_frame_alloc();
  if (!ctx->frame) {
    std::cerr << "Could not allocate frame" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  ctx->frame->format = ctx->codecContext->pix_fmt;
  ctx->frame->width = frameWidth;
  ctx->frame->height = frameHeight;
  
  if (av_frame_get_buffer(ctx->frame, 0) < 0) {
    std::cerr << "Could not allocate frame buffer" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  // Allocate packet
  ctx->packet = av_packet_alloc();
  if (!ctx->packet) {
    std::cerr << "Could not allocate packet" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  // Initialize swscale context for BGRA to YUV conversion
  // Metal returns BGRA format (B=byte0, G=byte1, R=byte2, A=byte3)
  ctx->swsContext = sws_getContext(
    frameWidth, frameHeight, AV_PIX_FMT_BGRA,
    frameWidth, frameHeight, AV_PIX_FMT_YUV420P,
    SWS_BILINEAR, nullptr, nullptr, nullptr
  );
  
  if (!ctx->swsContext) {
    std::cerr << "Could not create swscale context" << std::endl;
    cleanupEncoder();
    return false;
  }
  
  std::cout << "Started recording to: " << filename << " (" << frameWidth << "×" << frameHeight << "@" << frameRate << "fps)" << std::endl;
  return true;
}

bool VideoRecorder::addFrame(const void* pixels, int width, int height) {
  if (!recording || !ffmpegContext) {
    return false;
  }
  
  FFmpegContext* ctx = static_cast<FFmpegContext*>(ffmpegContext);
  
  if (width != frameWidth || height != frameHeight) {
    std::cerr << "Frame size mismatch!" << std::endl;
    return false;
  }
  
  // Make frame writable
  if (av_frame_make_writable(ctx->frame) < 0) {
    return false;
  }
  
  // Convert ARGB8888 to YUV420P
  const uint8_t* srcData[1] = {static_cast<const uint8_t*>(pixels)};
  int srcLinesize[1] = {width * 4}; // ARGB = 4 bytes per pixel
  
  sws_scale(ctx->swsContext, srcData, srcLinesize, 0, height,
            ctx->frame->data, ctx->frame->linesize);
  
  // Set frame timestamp
  ctx->frame->pts = ctx->frameCount++;
  
  // Encode frame
  int ret = avcodec_send_frame(ctx->codecContext, ctx->frame);
  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    std::cerr << "Error sending frame: " << errbuf << std::endl;
    return false;
  }
  
  // Receive packets
  while (ret >= 0) {
    ret = avcodec_receive_packet(ctx->codecContext, ctx->packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
      std::cerr << "Error encoding frame: " << errbuf << std::endl;
      return false;
    }
    
    // Rescale packet timestamp
    av_packet_rescale_ts(ctx->packet, ctx->codecContext->time_base, ctx->videoStream->time_base);
    ctx->packet->stream_index = ctx->videoStream->index;
    
    // Write packet
    av_interleaved_write_frame(ctx->formatContext, ctx->packet);
    av_packet_unref(ctx->packet);
  }
  
  return true;
}

void VideoRecorder::stopRecording() {
  if (!recording) {
    return;
  }
  
  FFmpegContext* ctx = static_cast<FFmpegContext*>(ffmpegContext);
  
  if (ctx && ctx->codecContext) {
    // Flush encoder
    int ret = avcodec_send_frame(ctx->codecContext, nullptr);
    while (ret >= 0) {
      ret = avcodec_receive_packet(ctx->codecContext, ctx->packet);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      } else if (ret < 0) {
        break;
      }
      
      av_packet_rescale_ts(ctx->packet, ctx->codecContext->time_base, ctx->videoStream->time_base);
      ctx->packet->stream_index = ctx->videoStream->index;
      av_interleaved_write_frame(ctx->formatContext, ctx->packet);
      av_packet_unref(ctx->packet);
    }
    
    // Write trailer
    av_write_trailer(ctx->formatContext);
    
    std::cout << "Stopped recording. Saved to: " << filename << std::endl;
  }
  
  cleanupEncoder();
  recording = false;
}

bool VideoRecorder::moveFile(const std::string& newPath) {
  if (filename.empty() || newPath.empty()) {
    return false;
  }
  
  // Use rename() which is atomic on the same filesystem
  if (std::rename(filename.c_str(), newPath.c_str()) == 0) {
    filename = newPath;
    return true;
  }
  
  // If rename fails (different filesystems), try copy + delete
  FILE* src = std::fopen(filename.c_str(), "rb");
  if (!src) {
    return false;
  }
  
  FILE* dst = std::fopen(newPath.c_str(), "wb");
  if (!dst) {
    std::fclose(src);
    return false;
  }
  
  // Copy file
  char buffer[8192];
  size_t bytes;
  bool success = true;
  while ((bytes = std::fread(buffer, 1, sizeof(buffer), src)) > 0) {
    if (std::fwrite(buffer, 1, bytes, dst) != bytes) {
      success = false;
      break;
    }
  }
  
  std::fclose(src);
  std::fclose(dst);
  
  if (success) {
    std::remove(filename.c_str()); // Delete original
    filename = newPath;
    return true;
  } else {
    std::remove(newPath.c_str()); // Clean up failed copy
    return false;
  }
}

void VideoRecorder::cleanupEncoder() {
  if (!ffmpegContext) {
    return;
  }
  
  FFmpegContext* ctx = static_cast<FFmpegContext*>(ffmpegContext);
  
  if (ctx) {
    if (ctx->swsContext) {
      sws_freeContext(ctx->swsContext);
      ctx->swsContext = nullptr;
    }
    
    if (ctx->packet) {
      av_packet_free(&ctx->packet);
    }
    
    if (ctx->frame) {
      av_frame_free(&ctx->frame);
    }
    
    if (ctx->codecContext) {
      avcodec_free_context(&ctx->codecContext);
    }
    
    if (ctx->formatContext) {
      if (!(ctx->formatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&ctx->formatContext->pb);
      }
      avformat_free_context(ctx->formatContext);
    }
    
    delete ctx;
    ffmpegContext = nullptr;
  }
}

