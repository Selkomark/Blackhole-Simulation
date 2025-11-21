#pragma once

#include <string>
#include <cstdint>

/**
 * Video recorder for capturing frames and encoding to video file with audio
 */
class VideoRecorder {
public:
  VideoRecorder();
  ~VideoRecorder();
  
  // Start recording to a file
  bool startRecording(const std::string& filename, int width, int height, int fps = 60, const std::string& audioFile = "");
  
  // Stop recording and finalize video file (mixes audio if provided)
  void stopRecording();
  
  // Add a frame to the video (ARGB8888 format, 4 bytes per pixel)
  bool addFrame(const void* pixels, int width, int height);
  
  // Check if currently recording
  bool isRecording() const { return recording; }
  
  // Get current output filename
  const std::string& getFilename() const { return filename; }
  
  // Move the recorded file to a new location (for save dialog)
  bool moveFile(const std::string& newPath);

private:
  bool recording;
  std::string filename;
  std::string audioFilePath;
  int frameWidth;
  int frameHeight;
  int frameRate;
  void* ffmpegContext; // Opaque pointer to FFmpeg context
  
  // Initialize FFmpeg encoder
  bool initializeEncoder();
  
  // Cleanup FFmpeg resources
  void cleanupEncoder();
  
  // Mux audio with video after recording
  bool muxAudioWithVideo();
};

