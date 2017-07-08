// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 17 15:47:01 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <vector>
#include <string>

// Apple imports:
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreServices/CoreServices.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

// yae:
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // stringFrom
  //
  static std::string
  stringFrom(CFStringRef cfStr)
  {
    std::string result;

    CFIndex strLen = CFStringGetLength(cfStr);
    CFIndex bufSize = CFStringGetMaximumSizeForEncoding(strLen + 1,
                                                        kCFStringEncodingUTF8);

    std::vector<char> buffer(bufSize + 1);
    char * buf = &buffer[0];

    if (CFStringGetCString(cfStr,
                           buf,
                           bufSize,
                           kCFStringEncodingUTF8))
    {
      result = buf;
    }

    return result;
  }

  //----------------------------------------------------------------
  // absoluteUrlFrom
  //
  std::string
  absoluteUrlFrom(const char * utf8_url)
  {
    std::string result(utf8_url);

#if !(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1060)
    CFStringRef cfStr = CFStringCreateWithCString(kCFAllocatorDefault,
                                                  utf8_url,
                                                  kCFStringEncodingUTF8);
    if (cfStr)
    {
      CFURLRef cfUrl = CFURLCreateWithString(kCFAllocatorDefault,
                                             cfStr,
                                             NULL);
      if (cfUrl)
      {
        CFErrorRef error = 0;
        CFURLRef cfUrlAbs = CFURLCreateFilePathURL(kCFAllocatorDefault,
                                                   cfUrl,
                                                   &error);
        if (cfUrlAbs)
        {
          CFStringRef cfStrAbsUrl = CFURLGetString(cfUrlAbs);
          result = stringFrom(cfStrAbsUrl);

          CFRelease(cfUrlAbs);
        }

        CFRelease(cfUrl);
      }

      CFRelease(cfStr);
    }
#endif

    return result;
  }

  //----------------------------------------------------------------
  // au_render_cb
  //
  // NOTE: see AURenderCallback (defined in AUComponent.h)
  //
  static OSStatus au_render_cb(// The client data that is provided either
                               // with the AURenderCallbackStruct
                               // or as specified with the Add API call
                               void * inRefCon,

                               // Flags used to describe more about the
                               // context of this call (pre or post in the
                               // notify case for instance)
                               AudioUnitRenderActionFlags * ioActionFlags,

                               // The times stamp associated with
                               // this call of audio unit render
                               const AudioTimeStamp * inTimeStamp,

                               // The bus numbeer associated with this call
                               // of audio unit render
                               UInt32 inBusNumber,

                               // The number of sample frames that will be
                               // represented in the audio data in the
                               // provided ioData parameter
                               UInt32 inNumberFrames,

                               // The AudioBufferList that will be used to
                               // contain the rendered or provided audio data
                               AudioBufferList * ioData)
  {
    static UInt32 t = 0;
    AudioStreamBasicDescription * sd = (AudioStreamBasicDescription *)inRefCon;
    float * dst = (float *)(ioData->mBuffers[0].mData);
    float * end = dst + inNumberFrames * sd->mChannelsPerFrame;
    while (dst < end)
    {
      for (UInt32 c = 0; c < sd->mChannelsPerFrame; c++, dst++)
      {
        float amp = fabsf(sinf(M_PI *
                               float(t * ((1 - c) * 3)) /
                               float(sd->mSampleRate)));
        *dst =
          sinf(M_PI * float((t * (c + 1) * 3) % 360) / 180.f) *
          (0.5 + 0.5 * amp);
      }

      t++;
    }

    return noErr;
  }

  //----------------------------------------------------------------
  // test_audio_output
  //
  void
  test_audio_output()
  {
    OSStatus err = noErr;

    ComponentDescription desc;
    memset(&desc, 0, sizeof(desc));
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    Component comp = FindNextComponent(NULL, &desc);

    AudioUnit audioUnit;
    memset(&audioUnit, 0, sizeof(audioUnit));

    err = OpenAComponent(comp, &audioUnit);
    YAE_ASSERT(!err);
    /*
    err = AudioUnitAddPropertyListener(audioUnit,
                                       kAudioOutputUnitProperty_IsRunning,
                                       startStopCallback,
                                       (void *)stream);
    */

    UInt32 value = kAudioConverterQuality_Max;
    err = AudioUnitSetProperty(audioUnit,
                               kAudioUnitProperty_RenderQuality,
                               kAudioUnitScope_Global,
                               0, // 0 == output
                               &value,
                               sizeof(value));
    YAE_ASSERT(!err);

    // FIXME:
    static const int channelCount = 2;
    static const int sampleRate = 44100;

    AudioStreamBasicDescription streamDesc;
    memset(&streamDesc, 0, sizeof(streamDesc));
    streamDesc.mFramesPerPacket = 1;
    streamDesc.mBitsPerChannel  = 8 * sizeof(float);
    streamDesc.mFormatFlags     = kAudioFormatFlagsNativeFloatPacked;
    streamDesc.mFormatID        = kAudioFormatLinearPCM;
    streamDesc.mSampleRate      = sampleRate;
    streamDesc.mBytesPerPacket  = sizeof(float) * channelCount;
    streamDesc.mBytesPerFrame   = sizeof(float) * channelCount;
    streamDesc.mChannelsPerFrame = channelCount;

    err = AudioUnitSetProperty(audioUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0, // 0 == output
                               &streamDesc,
                               sizeof(streamDesc));
    YAE_ASSERT(!err);
    /*
    // aim for 16 msec, roughly 60fps:
    UInt32 framesPerBuffer = (sampleRate * 16) / 1000;
    err = AudioUnitSetProperty(audioUnit,
                               kAudioUnitProperty_MaximumFramesPerSlice,
                               kAudioUnitScope_Output,
                               0, // 0 == output
                               &framesPerBuffer,
                               sizeof(&framesPerBuffer));
    YAE_ASSERT(!err);
    */
    AURenderCallbackStruct rcb;
    memset(&rcb, 0, sizeof(rcb));
    rcb.inputProc = &au_render_cb;
    rcb.inputProcRefCon = &streamDesc;

    err = AudioUnitSetProperty(audioUnit,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Output,
                               0, // 0 == output
                               &rcb,
                               sizeof(rcb));
    YAE_ASSERT(!err);

    // FIXME: IDK: is it necessary to set output channel map?
    // kAudioOutputUnitProperty_ChannelMap

    err = AudioUnitInitialize(audioUnit);
    YAE_ASSERT(!err);

    err = AudioOutputUnitStart(audioUnit);
    YAE_ASSERT(!err);

    sleep(5);

    err = AudioOutputUnitStop(audioUnit);
    YAE_ASSERT(!err);

    // FIXME: IDK: is it necessary to wait for audio unit to stop running?
    // kAudioOutputUnitProperty_IsRunning

    err = AudioUnitReset(audioUnit, kAudioUnitScope_Global, 0);
    YAE_ASSERT(!err);

    // clean up:
    CloseComponent(audioUnit);
  }
}
