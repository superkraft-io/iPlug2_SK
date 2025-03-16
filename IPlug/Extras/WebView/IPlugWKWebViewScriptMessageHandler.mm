 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView Library
  Copyright (c) 2024 Oliver Larkin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
 ==============================================================================
*/

#import "IPlugWKWebViewScriptMessageHandler.h"
#include "IPlugWebView.h"

#include "../../../../skxx/core/sk_common.hpp"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;
using namespace SK;

@implementation IPLUG_WKSCRIPTMESSAGEHANDLER

-(id) initWithIWebView:(IWebView*) webView
{
  self = [super init];
  
  if (self)
    mIWebView = webView;
  
  return self;
}

- (void)dealloc
{
  mIWebView = nullptr;
}

- (void) userContentController:(WKUserContentController*) userContentController didReceiveScriptMessage:(WKScriptMessage*) message
{
    SK_String _name = message.name;
    //SK_String _body = message.body;
  if ([[message name] isEqualToString:@"SK_IPC_Handler"])
  {
    NSDictionary* dict = (NSDictionary*) message.body;
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
    NSString* jsonString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    //mIWebView->OnMessageFromWebView([jsonString UTF8String]);
      
    nlohmann::json json = nlohmann::json::parse([jsonString UTF8String], nullptr, false);

    /**** SK START ****/

    bool isSK_IPC_call = json.contains("isSK_IPC_call");
    if (isSK_IPC_call) {
      SK_Communication_Config config{"sk:sb", SK_Communication_Packet_Type::sk_comm_pt_ipc, &json};

      SK_Global::onCommunicationRequest(&config, [&](const SK_String& ipcResponseData) {
        SK_String data = "sk_api.ipc.handleIncoming(" + ipcResponseData + ")";
        mIWebView->EvaluateJavaScript(data.c_str());
      }, NULL);
 
    return;
  }

  /**** SK END ****/
  }
/*
  if ([[message name] isEqualToString:@"callback"])
  {
    NSDictionary* dict = (NSDictionary*) message.body;
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
    NSString* jsonString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    mIWebView->OnMessageFromWebView([jsonString UTF8String]);
  }
 */
}

- (NSURL*) changeURLScheme:(NSURL*) url toScheme:(NSString*) newScheme
{
  NSURLComponents* components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:YES];
  components.scheme = newScheme;
  return components.URL;
}

- (void) webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){

    SK_Communication_Config config{"sk.sb", SK_Communication_Packet_Type::sk_comm_pt_web, (__bridge void *)urlSchemeTask.request};
    SK_Global::onCommunicationRequest(&config, NULL, [&](SK_Communication_Packet* packet) {
        if (packet == nullptr){
            return SK_Communication_Packet::packetFromWebRequest(urlSchemeTask.request, config.sender);
        }
        
        
        SK_Communication_Response_Web* responseObj = static_cast<SK_Communication_Response_Web*>(packet->response());
        SK_Communicaton_Response_Apple res = responseObj->getWebResponse();
        [urlSchemeTask didReceiveResponse:res.response];
        [urlSchemeTask didReceiveData:res.data];
        [urlSchemeTask didFinish];
        
        return packet;
    });
    
    return;
    
  /*NSString* customUrlScheme = [NSString stringWithUTF8String:mIWebView->GetCustomUrlScheme()];
  const BOOL useCustomUrlScheme = [customUrlScheme length];
  
  NSString* urlScheme = @"file:";
  
  if (useCustomUrlScheme)
  {
    urlScheme = [urlScheme stringByReplacingOccurrencesOfString:@"file" withString:customUrlScheme];
  }
  
  if (useCustomUrlScheme && [urlSchemeTask.request.URL.absoluteString containsString: urlScheme])
  {
      
      
    NSURL* customFileURL = urlSchemeTask.request.URL;
    NSURL* fileURL = [self changeURLScheme:customFileURL toScheme:@"file"];
    NSHTTPURLResponse* response = NULL;
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:fileURL.path])
    {
      NSData* data = [[NSData alloc] initWithContentsOfURL:fileURL];
      NSString* contentLengthStr = [[NSString alloc] initWithFormat:@"%lu", [data length]];
      NSString* contentTypeStr = @"text/plain";
      NSString* extStr = [[fileURL path] pathExtension];
      if ([extStr isEqualToString:@"html"]) contentTypeStr = @"text/html";
      else if ([extStr isEqualToString:@"css"]) contentTypeStr = @"text/css";
      else if ([extStr isEqualToString:@"js"]) contentTypeStr = @"text/javascript";
      else if ([extStr isEqualToString:@"jpg"]) contentTypeStr = @"image/jpeg";
      else if ([extStr isEqualToString:@"jpeg"]) contentTypeStr = @"image/jpeg";
      else if ([extStr isEqualToString:@"svg"]) contentTypeStr = @"image/svg+xml";
      else if ([extStr isEqualToString:@"json"]) contentTypeStr = @"text/json";

      NSDictionary* headerFields = [NSDictionary dictionaryWithObjects:@[contentLengthStr, contentTypeStr] forKeys:@[@"Content-Length", @"Content-Type"]];
      response = [[NSHTTPURLResponse alloc] initWithURL:customFileURL statusCode:200 HTTPVersion:@"HTTP/1.1" headerFields:headerFields];
      [urlSchemeTask didReceiveResponse:response];
      [urlSchemeTask didReceiveData:data];
    }
    else
    {
      response = [[NSHTTPURLResponse alloc] initWithURL:customFileURL statusCode:404 HTTPVersion:@"HTTP/1.1" headerFields:nil];
      [urlSchemeTask didReceiveResponse:response];
    }
    [urlSchemeTask didFinish];
  }*/
}

- (void) webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){
  
}

@end
