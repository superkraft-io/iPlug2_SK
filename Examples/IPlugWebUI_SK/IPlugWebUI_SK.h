#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"

#include "../../skxx/frameworks/iPlug2/sk_framework_iplug2.hpp"

using namespace iplug;


class IPlugWebUI_SK : public Plugin {
public:
    SK_Global* skg = nullptr;

    IPlugWebUI_SK(const InstanceInfo& info, int paramCount = 0, int presetCount = 0);
  
    bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
    bool CanNavigateToURL(const char* url);
    bool OnCanDownloadMIMEType(const char* mimeType) override;
    void OnFailedToDownloadFile(const char* path) override;
    void OnDownloadedFile(const char* path) override;
    void OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath) override;
    void OnIdle() override;

    void initSK();
private:
};
