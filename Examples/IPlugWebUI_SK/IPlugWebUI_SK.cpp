#include "IPlugWebUI_SK.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"

#include "../../skxx/core/sk_common.hpp"

using namespace SK;


IPlugWebUI_SK::IPlugWebUI_SK(const InstanceInfo& info, int paramCount, int presetCount) : Plugin(info, MakeConfig(paramCount, presetCount)) {

    #ifdef DEBUG
        SetEnableDevTools(true);
    #endif
  
    mEditorInitFunc = [&]() {
        EnableScroll(false);
    };
  
}


IPlugWebUI_SK::~IPlugWebUI_SK() {
    skg = nullptr;
}


bool IPlugWebUI_SK::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) {
    return false;
}



bool IPlugWebUI_SK::CanNavigateToURL(const char* url) {
    DBGMSG("Navigating to URL %s\n", url);

    return true;
}

bool IPlugWebUI_SK::OnCanDownloadMIMEType(const char* mimeType) {
    return std::string_view(mimeType) != "text/html";
}

void IPlugWebUI_SK::OnDownloadedFile(const char* path) {
    WDL_String str;
    str.SetFormatted(64, "Downloaded file to %s\n", path);
    LoadHTML(str.Get());
}

void IPlugWebUI_SK::OnFailedToDownloadFile(const char* path) {
    WDL_String str;
    str.SetFormatted(64, "Faild to download file to %s\n", path);
    LoadHTML(str.Get());
}

void IPlugWebUI_SK::OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath) {
    DesktopPath(localPath);
    localPath.AppendFormatted(MAX_WIN32_PATH_LEN, "/%s", fileName);
}

void IPlugWebUI_SK::OnIdle() {
    if (!getAcceptsTick()) return;

    Superkraft* sk = getSK();
    if (!sk) return;
    skg = sk->skg;

    SK_Global* _skg = sk->skg;
    if (!sk->skg) return;

    if (!sk->skg->framework_base){
        initSK();
        return;
    }
    
    
    if (!sk->isReady) {
        return;
    }
}

void IPlugWebUI_SK::initSK(){
    Superkraft* sk = getSK();
    SK_Global* skg = sk->skg;
    
    skg->OBJCPPSafeTicker = [&, sk, skg](){
        bool isRunningInMainThread = skg->threadPool->thisFunctionRunningInMainThread();
        if (isRunningInMainThread) skg->threadPool_processMainThreadTasks();
        
        SK_Framework_iPlug2_Base* framework_base = static_cast<SK_Framework_iPlug2_Base*>(skg->framework_base);
        if (!sk || !framework_base)
          return;
        
        framework_base->updateParamValues();
    };
    
    
    
    skg->appInitializer = new SK_App_Initializer(nlohmann::json{{"applicationWillFinishLaunching", true}}, [skg]() {
        void* ptr = skg->sb_ipc;
        return static_cast<SK_IPC*>(ptr);
    });
    
    
    skg->framework_base = new SK_Framework_iPlug2_Base(skg);
    SK_Framework_iPlug2_Base* framework_base = static_cast<SK_Framework_iPlug2_Base*>(skg->framework_base);
    framework_base->init(this);

    
    #if defined(SK_APP_TYPE_app)
      void* pView = mView;
    #elif defined(SK_APP_TYPE_au)
        void* pView = mView;
    #elif defined(SK_APP_TYPE_vst)
        void* pView = getMView();
    #endif

    skg->onMainWindowHWNDAcquired(pView, true);

    LoadFile(SK_String(SK_Base_URL + "/sk:sb/sk_sb.html").c_str());


    if (onPluginInitialized) onPluginInitialized();

    sk->isReady = true;
};
