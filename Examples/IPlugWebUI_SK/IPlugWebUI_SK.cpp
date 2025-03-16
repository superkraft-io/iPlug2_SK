#include "IPlugWebUI_SK.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"

#include "../../skxx/core/sk_common.hpp"

using namespace SK;


IPlugWebUI_SK::IPlugWebUI_SK(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
  
#ifdef DEBUG
  SetEnableDevTools(true);
#endif
  
  mEditorInitFunc = [&]()
  {
    //LoadIndexHtml(__FILE__, GetBundleID());
    LoadFile(SK_String(SK_Base_URL + "/sk:sb/sk_sb.html").c_str());
    EnableScroll(false);
  };
  
  MakePreset("One", -70.);
  MakePreset("Two", -30.);
  MakePreset("Three", 0.);
}

void IPlugWebUI_SK::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
    
  mOscillator.ProcessBlock(inputs[0], nFrames); // comment for audio in

  for (int s = 0; s < nFrames; s++)
  {
    outputs[0][s] = inputs[0][s] * mGainSmoother.Process(gain);
    outputs[1][s] = outputs[0][s]; // copy left    
  }  
}

void IPlugWebUI_SK::OnReset()
{
  auto sr = GetSampleRate();
  mOscillator.SetSampleRate(sr);
  mGainSmoother.SetSmoothTime(20., sr);
}

bool IPlugWebUI_SK::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  /*if (msgTag == kMsgTagButton1)
  {
    EditorResize(300, 300);
  }
  else if (msgTag == kMsgTagButton2)
  {
    EditorResize(600, 600);
  }
  else if (msgTag == kMsgTagButton3)
  {
    EditorResize(1024, 768);
  }
  else if (msgTag == kMsgTagBinaryTest)
  {
    auto uint8Data = reinterpret_cast<const uint8_t*>(pData);
    DBGMSG("Data Size %i bytes\n",  dataSize);
    DBGMSG("Byte values: %i, %i, %i, %i\n", uint8Data[0], uint8Data[1], uint8Data[2], uint8Data[3]);
  }
   */

  return false;
}

void IPlugWebUI_SK::OnParamChange(int paramIdx)
{
  DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
}

void IPlugWebUI_SK::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  msg.PrintMsg();
  SendMidiMsg(msg);
}

bool IPlugWebUI_SK::CanNavigateToURL(const char* url)
{
  DBGMSG("Navigating to URL %s\n", url);

  return true;
}

bool IPlugWebUI_SK::OnCanDownloadMIMEType(const char* mimeType)
{
  return std::string_view(mimeType) != "text/html";
}

void IPlugWebUI_SK::OnDownloadedFile(const char* path)
{
  WDL_String str;
  str.SetFormatted(64, "Downloaded file to %s\n", path);
  LoadHTML(str.Get());
}

void IPlugWebUI_SK::OnFailedToDownloadFile(const char* path)
{
  WDL_String str;
  str.SetFormatted(64, "Faild to download file to %s\n", path);
  LoadHTML(str.Get());
}

void IPlugWebUI_SK::OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath)
{
  DesktopPath(localPath);
  localPath.AppendFormatted(MAX_WIN32_PATH_LEN, "/%s", fileName);
}
