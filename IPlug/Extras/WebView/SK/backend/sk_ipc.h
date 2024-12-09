#pragma once

#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "wdlstring.h"
#include <functional>
#include <memory>

#include "json.hpp"
#include <string>


#include "sk_base.h"

BEGIN_SK_NAMESPACE

/** IWebView is a base interface for hosting a platform web view inside an IPlug plug-in's UI */
class SK_IPC
{
public:
  std::string sender_id = "SK_iPlug2_WebView_Backend";
  long long msg_id = 0;

  /** Returns a standard OK IPC message*/
  static inline const std::string OK = "{}";

  /** Returns an ERROR IPC object
   * @param error Error code
   * @param message A human readable message
   * @return Returns a stringified JSON object, e.g {error: "failed", message: "This request failed"}*/
  static inline const std::string Error(std::string error, std::string message = "")
  {
    nlohmann::json json;

    json["error"] = error;
    json["message"] = message;

    return json.dump();
  };

  using SK_IPC_FrontendCallback = std::function<void(nlohmann::json, std::string& responseData)>;
  using SK_IPC_BackendCallback = std::function<void(nlohmann::json)>;

  SK_IPC_BackendCallback onSendToFrontend;
  SK_IPC_BackendCallback onMessage;

  
  nlohmann::json handle_IPC_Msg(const nlohmann::json& json);

  /** Returns an event type if it exists
   * @param event_id Name of the event
   * @return "always" for a standard event, "once" for a one-time event, "" (empty string) if the event does not exist*/
  std::string SK_IPC::eventExists(const std::string& event_id);

  /** Adds an event that is fired when the frontend requests a response with the specified event ID
   * @param event_id Name of the event
   * @param callback Callback with the event data
   * @return A string representing the event message ID*/
  void on(const std::string& event_id, SK_IPC_FrontendCallback callback);

  
  
  /** Removes an event
   * @param event_id Name of the event*/
  void off(const std::string& event_id);

  
  /** Adds a one-time event that is fired when the frontend requests a response with the specified event ID. A one-time event is automatically removed once it has been fired.
   * @param event_id Name of the event
   * @param callback Callback with the event data*/
  void once(const std::string& event_id, SK_IPC_FrontendCallback callback);

  
  /** Makes a request to the frontend and awaits a response (currently indefinitely)
   * @param event_id Name of the event
   * @param data Data to send
   * @param cb Callback of the response*/
  void request(std::string event_id, nlohmann::json data, SK_IPC_BackendCallback cb);

  /** Sends a response-less message to the frontend. This function does NOT expect or wait for a response.
   * @param data Data to send*/
  void message(nlohmann::json data);

private:
  std::unordered_map<std::string, SK_IPC_BackendCallback> awaitList;
  std::unordered_map<std::string, SK_IPC_FrontendCallback> listeners;
  std::unordered_map<std::string, SK_IPC_FrontendCallback> listeners_once;

  void handleRequest(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data, std::string& responseData);
  void handleResponse(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data);
  void handleMessage(const nlohmann::json& json);

  std::string sendToFE(std::string event_id, nlohmann::json data, std::string type, SK_IPC_BackendCallback cb);
};

END_SK_NAMESPACE
