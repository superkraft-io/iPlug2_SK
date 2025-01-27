#include "IPlugWebUI_SK.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"
#include "../APP/IPlugAPP_host.h"

#include "../../skxx/core/sk_common.hxx"

using namespace SK;

IPlugWebUI_SK::IPlugWebUI_SK(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  /**** SK START ****/
  SK_Common::getMainWindowSize = [&]() {
    SK_Point size{GetEditorWidth(), GetEditorHeight()};
    return size;
  };

  SK_Common::setMainWindowSize = [&](int w, int h) {
    SetEditorSize(w, h);
    OnParentWindowResize(w, h);


    SK_Common::resizeAllMianWindowView(0, 0, w, h, 1);

    #if defined(SK_OS_windows)
      float scale = getHWNDScale(SK_Common::mainWindowHWND);
      SetWindowPos(SK_Common::mainWindowHWND, NULL, 0, 0, w * scale, h * scale, SWP_NOMOVE | SWP_NOZORDER);
      SendMessage(SK_Common::mainWindowHWND, WM_SIZE, SIZE_RESTORED, MAKELPARAM(w * scale, h * scale));
    #endif
  };

  SK_Common::onMainWindowHWNDAcquired = [&](HWND hwnd) {
    SK_Window* wnd = sk()->wndMngr.newWindow([&](SK_Window* wnd) {
      wnd->config["width"] = GetEditorWidth();
      wnd->config["height"] = GetEditorHeight();

      wnd->tag = "sb";
      wnd->config["visible"] = true;
      wnd->hwnd = SK_Common::mainWindowHWND;
      SK_Common::updateWebViewHWNDListForView(wnd->windowClassName);
      SK_Common::sb_ipc = &wnd->ipc;

      sk()->comm.sb_ipc = &wnd->ipc;

      SK_Common::sb_ipc->on("valid_event_id", [](nlohmann::json data, SK_Communication_Packet* packet) {
        nlohmann::json json;

        std::string frontend_message = std::string(data["key"]);

        json["backend_said"] = "hello frontend :)";
        packet->response()->JSON(json);
      });

      SK_Common::sb_ipc->once("valid_event_id_once", [](nlohmann::json data, SK_Communication_Packet* packet) {
        nlohmann::json json;

        std::string frontend_message = SK_String(data["key"]);

        json["backend_said"] = "hello frontend :) deleting this event now";
        packet->response()->JSON(json);
      });


      SK_Common::sb_ipc->onMessage = [this](const SK_String& sender, SK_Communication_Packet* packet) {
        std::string action = packet->data["action"];

        if (action == "reqFromBE")
        {
          nlohmann::json be_data;
          be_data["this_is"] = "a backend request :)";

          SK_Common::sb_ipc->request("sk.hb", "sk.sb", "requestFromBackend", be_data, [](const SK_String& sender, SK_Communication_Packet* packet) {
            std::string key = std::string(packet->data["key"]);
            DBGMSG("key = %s\n", key.c_str());
          });
        }
        else if (action == "msgFromBE")
        {
          nlohmann::json be_data;
          be_data["this_is"] = "a message from backend :)";

          SK_Common::sb_ipc->message(be_data);
        }
        else
        {
          DBGMSG("data = %s\n", packet->data.dump().c_str());
        }
      };
    });
  };

    
    
    
    


  SK_Common::onWebViewReady = [&](void* webview, bool isHardBackend) {
    sk()->wvinit.init(webview, isHardBackend);
  };


  SK_IPC_v2::onSendToFrontend = [&](const SK_String& target, const SK_String& data) {
    SK_String str = "sk_api.ipc.handleIncoming(" + data + ")";

    if (target == "sk.sb")
    {
      EvaluateJavaScript(str.c_str());
    }
    else
    {
      SK_Window* view = sk()->wndMngr.findWindowByTag(target);
      view->webview.evaluateScript(str, NULL);
    }
  };

   
  /**** SK END ****/


  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
  
#if defined DEBUG || defined _DEBUG
  SetEnableDevTools(true);
#endif
  
  mEditorInitFunc = [&]()
  {
    //LoadIndexHtml(__FILE__, GetBundleID());
    LoadFile("https://sk.sb/sk_sb.html");
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
