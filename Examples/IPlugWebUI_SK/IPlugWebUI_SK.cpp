#include "IPlugWebUI_SK.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"

using namespace SK;

IPlugWebUI_SK::IPlugWebUI_SK(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{

  
  /**** SK START ****/

  sk()->ipc.onSendToFrontend = [this](std::string data) {
    std::string str = "SK_iPlug2.ipc.handleIncoming(" + data + ")";
    EvaluateJavaScript(str.c_str());
  };


  sk()->ipc.on("valid_event_id", [](nlohmann::json data, SK_String& responseData) {
    nlohmann::json json;

    std::string frontend_message = std::string(data["key"]);

    json["backend_said"] = "hello frontend :)";
    responseData = json.dump();
  });

  sk()->ipc.once("valid_event_id_once", [](nlohmann::json data, SK_String& responseData) {
    nlohmann::json json;

    std::string frontend_message = SK_String(data["key"]);

    json["backend_said"] = "hello frontend :) deleting this event now";
    responseData = json.dump();
  });


  sk()->ipc.onMessage = [this](nlohmann::json data) {
    std::string action = data["action"];

    if (action == "reqFromBE")
    {
      nlohmann::json be_data;
      be_data["this_is"] = "a backend request :)";

      sk()->ipc.request("requestFromBackend", be_data, [](nlohmann::json data) {
        std::string key = std::string(data["key"]);
        DBGMSG("key = %s\n", key.c_str());
      });
    }
    else if (action == "msgFromBE")
    {
      nlohmann::json be_data;
      be_data["this_is"] = "a message from backend :)";

      sk()->ipc.message(be_data);
    }
    else
    {
      DBGMSG("data = %s\n", data.dump().c_str());
    }
  };

   
  /**** SK END ****/


  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
  
#ifdef DEBUG
  SetEnableDevTools(true);
#endif
  
  mEditorInitFunc = [&]()
  {
    LoadIndexHtml(__FILE__, GetBundleID());
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
  if (msgTag == kMsgTagButton1)
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
