#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"

#include "../../sk_project.hpp"

using namespace iplug;

const int kNumPresets = 3;

enum EParams
{
  kGain = 0,
  kBoolean,
  kInteger,
  kDouble,
  kList,
  kFrequency,
  kPercent,
  kMilliseconds,
  kNumParams
};

enum EMsgTags
{
  kMsgTagButton1 = 0,
  kMsgTagButton2 = 1,
  kMsgTagButton3 = 2,
  kMsgTagBinaryTest = 3
};

class IPlugWebUI_SK final : public Plugin
{
public:
    
  IPlugWebUI_SK(const InstanceInfo& info);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnParamChange(int paramIdx) override;
  bool CanNavigateToURL(const char* url);
  bool OnCanDownloadMIMEType(const char* mimeType) override;
  void OnFailedToDownloadFile(const char* path) override;
  void OnDownloadedFile(const char* path) override;
  void OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath) override;
  void OnIdle() override;

  void initSK();
private:
  float mLastPeak = 0.;
  FastSinOscillator<sample> mOscillator {0., 440.};
  LogParamSmooth<sample, 1> mGainSmoother;
};
