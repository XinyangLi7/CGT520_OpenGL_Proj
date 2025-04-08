#include "AudioProcessing.h"
#include "InitShader.h"
#include <mutex>
#include <iostream>
#include <glm/glm.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

std::mutex buffer_mutex;
std::vector<float> audio_buffer;
int max_buffer_size = 10000;
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

void AudioProcessing::Init(const std::string& filename)
{
   mAudioFilename = filename;
   //Init miniaudio
   ma_result result = ma_decoder_init_file(mAudioFilename.c_str(), NULL, &mAudioDecoder);
   if (result != MA_SUCCESS)
   {
      printf("Could not load file: %s\n", mAudioFilename.c_str());
   }

   mAudioConfig = ma_device_config_init(ma_device_type_playback);
   mAudioConfig.playback.format = mAudioDecoder.outputFormat;   // Set to ma_format_unknown to use the device's native format.
   mAudioConfig.playback.channels = mAudioDecoder.outputChannels;      // Set to 0 to use the device's native channel count.
   mAudioConfig.sampleRate = mAudioDecoder.outputSampleRate;           // Set to 0 to use the device's native sample rate.
   mAudioConfig.dataCallback = data_callback;   // This function will be called when miniaudio needs more data.
   mAudioConfig.pUserData = &mAudioDecoder;

   if (ma_device_init(NULL, &mAudioConfig, &mAudioDevice) != MA_SUCCESS)
   {
      std::cout << "Init failed" << std::endl;
   }

   if (ma_device_start(&mAudioDevice) != MA_SUCCESS)
   {
      printf("Failed to start playback device.\n");
      ma_device_uninit(&mAudioDevice);
      ma_decoder_uninit(&mAudioDecoder);
   }

   //Create compute shader
   mProcessShader = InitShader(mProcessShaderFilename.c_str());
   mFftShader = InitShader(mFftShaderFilename.c_str());

   //Create buffers
   glGenBuffers(2, mRealBuffer);
   glGenBuffers(2, mComplexBuffer);

   for(int i=0; i<2; i++)
   {
      const GLuint flags = 0;
      const int real_size = sizeof(float)*mTimeWindow;
      const int complex_size = 2*real_size;

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRealBuffer[i]);
      glNamedBufferStorage(mRealBuffer[i], real_size, nullptr, GL_DYNAMIC_STORAGE_BIT);

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mComplexBuffer[i]);
      glNamedBufferStorage(mComplexBuffer[i], complex_size, nullptr, flags);
   }
}

void AudioProcessing::Compute()
{
   extern std::vector<float> audio_buffer;
   extern std::mutex buffer_mutex;

   int safety = 0;
   const int max_safety = 10;
   //Pull last audio data into signal buffer
   int new_samples = mTimeWindow - mWindowOverlap;
   while (audio_buffer.size() >= new_samples && safety < max_safety)
   {
      safety++;
      buffer_mutex.lock();
      int n = audio_buffer.size();
      if (n > max_buffer_size)
      {
         //drop frames
         audio_buffer.clear();
         buffer_mutex.unlock();
         return;
      }
      //push new samples to end of signal buffer
      mSignal.insert(mSignal.end(), audio_buffer.begin(), audio_buffer.begin() + new_samples);
      //rotate old samples out of signal buffer
      std::rotate(mSignal.begin(), mSignal.begin() + new_samples, mSignal.end());
      //resize signal back to window size
      mSignal.resize(mTimeWindow);

      //rotate the old samples out of the audio buffer
      std::rotate(audio_buffer.begin(), audio_buffer.begin() + new_samples, audio_buffer.end());
      audio_buffer.resize(audio_buffer.size() - new_samples);
      //printf("Compute: audio_buffer.size() = %d\n", audio_buffer.size());
      buffer_mutex.unlock();

      const int offset = 0;
      glNamedBufferSubData(mRealBuffer[0], offset, sizeof(float) * mSignal.size(), mSignal.data());
      SignalFft();
   }
}

void AudioProcessing::SignalFft()
{
   //Audio initially in mRealBuffer[0].
   const int offset = 0;
   const int real_size = mTimeWindow*sizeof(float);
   const int complex_size = 2*real_size;
   const int mode_loc = 0;

   glUseProgram(mProcessShader);

   if (mEnableWindowing)
   {
      glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, mRealBuffer[0], offset, real_size);
      glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, mRealBuffer[1], offset, real_size);
      
      glProgramUniform1i(mProcessShader, mode_loc, 4); //mode = 4, windowing
      glDispatchCompute(1,1,1);

      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      std::swap(mRealBuffer[0], mRealBuffer[1]);
   }

   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mComplexBuffer[0], offset, complex_size);
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, mComplexBuffer[1], offset, complex_size);
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, mRealBuffer[0], offset, real_size);
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, mRealBuffer[1], offset, real_size);

   glProgramUniform1i(mProcessShader, mode_loc, 0); //mode = 0, real to complex
   glDispatchCompute(1, 1, 1);

   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
   std::swap(mComplexBuffer[0], mComplexBuffer[1]);

   //Do fft
   glUseProgram(mFftShader);
   
   const int N_loc = 0;
   const int Ns_loc = 1;

   int w = mTimeWindow;
   int log_w = glm::ceil(glm::log2(float(w)));

   glProgramUniform1i(mFftShader, N_loc, w);
   
   for (int i = 0; i < log_w; i++)
   {
      int step = 2 << i;
      glProgramUniform1i(mFftShader, Ns_loc, step);

      glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mComplexBuffer[0], offset, complex_size);
      glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, mComplexBuffer[1], offset, complex_size);
      glDispatchCompute(1, 1, 1);

      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      std::swap(mComplexBuffer[0], mComplexBuffer[1]);
   }
}

void AudioProcessing::BindFftForRead(int binding)
{
   const int offset = 0;
   const int complex_size = mTimeWindow * sizeof(float); //since real fft is symmetrical, we only bind half the values
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, mComplexBuffer[0], offset, complex_size);
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
   ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
   if (pDecoder == NULL)
   {
      return;
   }

   ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

   const ma_format format_out = ma_format_f32;
   const ma_dither_mode dither_mode = ma_dither_mode_none;

   buffer_mutex.lock();
   int n = audio_buffer.size();
   if (n > max_buffer_size)
   {
      //drop frames
      audio_buffer.clear();
      buffer_mutex.unlock();
      return;
   }
   audio_buffer.resize(n + frameCount);
   ma_pcm_convert(audio_buffer.data() + n, format_out, pOutput, pDecoder->outputFormat, frameCount, dither_mode);
   buffer_mutex.unlock();

   ma_uint64 remaining_frames = 0;
   ma_decoder_get_available_frames(pDecoder, &remaining_frames);
   if (remaining_frames == 0)
   {
      ma_decoder_seek_to_pcm_frame(pDecoder, 0);
   }

   //printf("audio callback: audio_buffer.size() = %d\n", audio_buffer.size());
}