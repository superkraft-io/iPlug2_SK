#include "sk_ipc.h"
#include <string>
#include <memory>

using namespace SK;

nlohmann::json SK_IPC::handle_IPC_Msg(const nlohmann::json& payload)
{

  std::string msg_id   = std::string(payload["msg_id"]);
  std::string type     = std::string(payload["type"]);
  std::string sender   = std::string(payload["sender"]);
  std::string target   = std::string(payload["target"]);
  std::string event_id = std::string(payload["event_id"]);
  nlohmann::json data  = payload["data"];


  if (type == "request"){
    nlohmann::json SK_iPlug2_IPC_packet;
    SK_iPlug2_IPC_packet["msg_id"] = payload["msg_id"];
    SK_iPlug2_IPC_packet["type"] = "response";
    SK_iPlug2_IPC_packet["sender"] = "iPlug2_backend";
    SK_iPlug2_IPC_packet["target"] = sender;
    SK_iPlug2_IPC_packet["event_id"] = payload["event_id"];

    std::string responseData = "{\"error\":\"invalid_ipc_request\"}";

    handleRequest(msg_id, type, sender, event_id, data, responseData);

    SK_iPlug2_IPC_packet["data"] = responseData;

    return SK_iPlug2_IPC_packet;
  }
  else if (type == "response"){
    handleResponse(msg_id, type, sender, event_id, data);
    return NULL;
  }
  else if (type == "message"){
    handleMessage(data);
    return NULL;
  }
}

std::string SK_IPC::eventExists(const std::string& event_id)
{
  auto listener = listeners[event_id];
  auto listener_once = listeners_once[event_id];

  if (listener != NULL) return "always";
  if (listener_once != NULL) return "once";

  return "";
}

void SK_IPC::on(const std::string& event_id, SK_IPC_FrontendCallback callback)
{
  if (eventExists(event_id) != "") return;

  listeners[event_id] = callback;
}

void SK_IPC::off(const std::string& event_id)
{
  listeners.erase(event_id);
  listeners_once.erase(event_id);
}

void SK_IPC::once(const std::string& event_id, SK_IPC_FrontendCallback callback)
{
  if (eventExists(event_id) != "") return;

  listeners_once[event_id] = callback;
}


void SK_IPC::handleRequest(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data, std::string& responseData)
{
  auto listener = listeners[event_id];
  auto listener_once = listeners_once[event_id];

  bool exists = false;

  if (listener != NULL){
    exists = true;
    listener(data, responseData);
  } else {
    if (listener_once != NULL){
      exists = true;
      listener_once(data, responseData);
      off(event_id);
    }
  }

  if (!exists){
    responseData = SK_IPC::Error("unknown_listener", "A listener with the event ID \"" + event_id + "\" does not exist.");
  }
};

void SK_IPC::handleResponse(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data)
{
  SK_IPC_BackendCallback awaiter = awaitList[msg_id];
  if (awaiter != NULL) awaiter(data);
};

void SK_IPC::handleMessage(const nlohmann::json& json)
{
  if (onMessage != NULL) onMessage(json);
};



std::string SK_IPC::sendToFE(std::string event_id, nlohmann::json data, std::string type, SK_IPC_BackendCallback cb)
{
   std::string _type = "request";
  if (type != "") _type = type;


  if (event_id == "") throw "[SK_iPlug2 IPC.sendToFE] Event ID cannot be empty";

  msg_id++;

  nlohmann::json req;

  req["msg_id"] = std::to_string(msg_id);
  req["type"] = _type;
  req["sender"] = sender_id;
  req["target"] = "SK_iPlug2_frontend";
  req["event_id"] = event_id;
  req["data"] = data;
  

  if (_type == "request") {
    awaitList[req["msg_id"]] = cb;
  }

  onSendToFrontend(req.dump());

  return req["msg_id"];
};

void SK_IPC::request(std::string event_id, nlohmann::json data, SK_IPC_BackendCallback cb) {
  sendToFE(event_id, data, "request", cb);
};

void SK_IPC::message(nlohmann::json data) {
  sendToFE("SK_iPlug2_IPC_Message", data, "message", NULL);
};
