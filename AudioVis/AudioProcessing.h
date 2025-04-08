#pragma once
#include <GL/glew.h>
#include <vector>
#include <string>
#include "miniaudio.h"

struct AudioProcessing
{
   ma_device_config mAudioConfig;
   ma_device mAudioDevice;
   ma_decoder mAudioDecoder;

   std::string mAudioFilename;

   std::vector<float> mSignal;
   std::vector<float> mWindowedSignal;
   std::vector<float> mFft;

   int mTimeWindow = 256; //must be power of 2, max value = 1024
   int mWindowOverlap = mTimeWindow / 2;
   bool mEnableWindowing = true;

   GLuint mProcessShader = -1;
   std::string mProcessShaderFilename = "shaders/fft_buffer_map_cs.glsl";

   GLuint mFftShader = -1;
   std::string mFftShaderFilename = "shaders/buffer_fft_cs.glsl";

   GLuint mRealBuffer[2];
   GLuint mComplexBuffer[2];

   void Init(const std::string& filename);
   void Compute();
   void SignalFft();
   void BindFftForRead(int binding);
};