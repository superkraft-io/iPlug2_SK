/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
 
#import <Cocoa/Cocoa.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>

#include "config.h"   // This is your plugin's config.h.
#include "IPlugAPIBase.h"


#include "../../skxx/core/sk_common.hpp"
#include "../../skxx/core/superkraft.hpp"
#include "../../sk_project.hpp"

using namespace iplug;

static const AudioUnitPropertyID kIPlugObjectPropertyID = UINT32_MAX-100;

@interface AUV2_VIEW_CLASS : NSObject <AUCocoaUIBase>
{
  IPlugAPIBase* mPlug;
}
- (id) init;
- (NSView*) uiViewForAudioUnit: (AudioUnit) audioUnit withSize: (NSSize) preferredSize;
- (unsigned) interfaceVersion;
- (NSString*) description;
@end

@implementation AUV2_VIEW_CLASS

- (id) init
{
  TRACE  
  mPlug = nullptr;
  
  
  return [super init];
}

- (NSView*) uiViewForAudioUnit: (AudioUnit) audioUnit withSize: (NSSize) preferredSize
{
  TRACE

  void* pointers[1];
  UInt32 propertySize = sizeof (pointers);
  
  if (AudioUnitGetProperty (audioUnit, kIPlugObjectPropertyID,
                            kAudioUnitScope_Global, 0, pointers, &propertySize) == noErr)
  {
    mPlug = (IPlugAPIBase*) pointers[0];
    
    Superkraft* sk = mPlug->getSK();
    
    if (mPlug)
    {
      if (mPlug->HasUI())
      {
        
        
        sk->skg->appInitializer = new SK_App_Initializer(nlohmann::json{{"applicationWillFinishLaunching", true}}, [sk]() {
          void* ptr = sk->skg->sb_ipc;
          return static_cast<SK_IPC_v2*>(ptr);
        });
        
        sk->skg->project = new SK_Project(sk->skg);
        (static_cast<SK_Project*>(sk->skg->project))->init(mPlug);
        
        
#if __has_feature(objc_arc)
        NSView* pView = (__bridge NSView*) mPlug->OpenWindow(nullptr);
#else
        NSView* pView = (NSView*) mPlug->OpenWindow(nullptr);
#endif
        if (pView) {
          dispatch_async(dispatch_get_main_queue(), ^{
            if (pView.window) {
              sk->skg->onMainWindowHWNDAcquired((__bridge void*)pView, true);
            }
          });
        }
        
        return pView;
      }
    }
  }
  return 0;
}

- (unsigned) interfaceVersion
{
  return 0;
}

- (NSString*) description
{
  return [NSString stringWithUTF8String:PLUG_NAME " View"];
}

@end


