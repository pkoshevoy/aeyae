// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Aug  8 09:13:01 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <assert.h>
#include <stdlib.h>

// apple:
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreServices/CoreServices.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

// yaeui:
#include "yaeAudioUnitRenderer.h"


//----------------------------------------------------------------
// yae_au_ctx_t
//
typedef struct yae_au_ctx_t
{
  void * owner_;
  TAuCtxPull pull_;
  TAuCtxStop stop_;
  AudioUnit au_;
  AudioStreamBasicDescription sd_;
} yae_au_ctx_t;

//----------------------------------------------------------------
// yae_au_ctx_create
//
void *
yae_au_ctx_create(void * owner,
                  TAuCtxPull pull,
                  TAuCtxStop stop)
{
  yae_au_ctx_t * ctx = (yae_au_ctx_t *)malloc(sizeof(yae_au_ctx_t));
  memset(ctx, 0, sizeof(*ctx));
  ctx->owner_ = owner;
  ctx->pull_ = pull;
  ctx->stop_ = stop;
  return ctx;
}

//----------------------------------------------------------------
// yae_au_ctx_destroy
//
void
yae_au_ctx_destroy(void ** context)
{
  yae_au_ctx_t * ctx = (yae_au_ctx_t *)(*context);
  if (ctx)
  {
    yae_au_ctx_stop(ctx);
    free(ctx);
    *context = NULL;
  }
}

//----------------------------------------------------------------
// yae_au_ctx_stop
//
void
yae_au_ctx_stop(void * context)
{
  yae_au_ctx_t * ctx = (yae_au_ctx_t *)context;
  if (ctx->au_)
  {
    AudioOutputUnitStop(ctx->au_);
    AudioUnitReset(ctx->au_, kAudioUnitScope_Global, 0);
    CloseComponent(ctx->au_);
    memset(&ctx->au_, 0, sizeof(ctx->au_));
    memset(&ctx->sd_, 0, sizeof(ctx->sd_));
  }
}

//----------------------------------------------------------------
// yae_au_ctx_render_cb
//
static OSStatus
callback(// The client data that is provided either
         // with the AURenderCallbackStruct
         // or as specified with the Add API call
         void * userData,

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
  yae_au_ctx_t * ctx = (yae_au_ctx_t *)userData;
  assert(ioData->mNumberBuffers == 1);
  assert(ioData->mBuffers[0].mNumberChannels == ctx->sd_.mChannelsPerFrame);

  bool planar = !(ctx->sd_.mFormatFlags & kAudioFormatFlagsNativeFloatPacked);

  if (ctx->pull_(ctx->owner_,
                 ioData->mBuffers[0].mData,
                 inNumberFrames,
                 ctx->sd_.mChannelsPerFrame,
                 planar))
  {
    return noErr;
  }

  ctx->stop_(ctx->owner_);
  return noErr;
}

//----------------------------------------------------------------
// yae_au_ctx_open_stream
//
bool
yae_au_ctx_open_stream(void * context,
                       int sample_rate,
                       int num_channels)
{
  OSStatus err = noErr;
  yae_au_ctx_t * ctx = (yae_au_ctx_t *)context;

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
  ComponentDescription desc;
#else
  AudioComponentDescription desc;
#endif

  memset(&desc, 0, sizeof(desc));
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
  Component comp = FindNextComponent(NULL, &desc);
#else
  AudioComponent comp = AudioComponentFindNext(NULL, &desc);
#endif

  memset(&ctx->au_, 0, sizeof(ctx->au_));

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
  err = OpenAComponent(comp, &ctx->au_);
#else
  err = AudioComponentInstanceNew(comp, &ctx->au_);
#endif

  if (err)
  {
    return false;
  }

  UInt32 value = kAudioConverterQuality_Max;
  err = AudioUnitSetProperty(ctx->au_,
                             kAudioUnitProperty_RenderQuality,
                             kAudioUnitScope_Global,
                             0, // 0 == output
                             &value,
                             sizeof(value));
  if (err)
  {
    return false;
  }

  memset(&ctx->sd_, 0, sizeof(ctx->sd_));
  ctx->sd_.mFramesPerPacket  = 1;
  ctx->sd_.mBitsPerChannel   = sizeof(float) * 8;
  ctx->sd_.mFormatFlags      = kAudioFormatFlagsNativeFloatPacked;
  ctx->sd_.mFormatID         = kAudioFormatLinearPCM;
  ctx->sd_.mSampleRate       = sample_rate;
  ctx->sd_.mChannelsPerFrame = num_channels;
  ctx->sd_.mBytesPerPacket   = sizeof(float) * ctx->sd_.mChannelsPerFrame;
  ctx->sd_.mBytesPerFrame    = sizeof(float) * ctx->sd_.mChannelsPerFrame;

  err = AudioUnitSetProperty(ctx->au_,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Input,
                             0, // 0 == output
                             &ctx->sd_,
                             sizeof(ctx->sd_));
  if (err)
  {
    AudioUnitReset(ctx->au_, kAudioUnitScope_Global, 0);
    CloseComponent(ctx->au_);
    memset(&ctx->au_, 0, sizeof(ctx->au_));
    return false;
  }

  AURenderCallbackStruct rcb;
  memset(&rcb, 0, sizeof(rcb));
  rcb.inputProc = &callback;
  rcb.inputProcRefCon = ctx;

  err = AudioUnitSetProperty(ctx->au_,
                             kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Output,
                             0, // 0 == output
                             &rcb,
                             sizeof(rcb));
  if (err)
  {
    return false;
  }

  err = AudioUnitInitialize(ctx->au_);
  if (err)
  {
    AudioUnitReset(ctx->au_, kAudioUnitScope_Global, 0);

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
    CloseComponent(ctx->au_);
#else
    AudioComponentInstanceDispose(ctx->au_);
#endif

    memset(&ctx->au_, 0, sizeof(ctx->au_));
    return false;
  }

  err = AudioOutputUnitStart(ctx->au_);
  if (err)
  {
    AudioUnitReset(ctx->au_, kAudioUnitScope_Global, 0);

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
    CloseComponent(ctx->au_);
#else
    AudioComponentInstanceDispose(ctx->au_);
#endif

    memset(&ctx->au_, 0, sizeof(ctx->au_));
    return false;
  }

  return true;
}
