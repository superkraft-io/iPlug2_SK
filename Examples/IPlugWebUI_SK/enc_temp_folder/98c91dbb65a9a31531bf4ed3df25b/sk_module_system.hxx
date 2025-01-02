#pragma once

#include "../core/sk_common.hxx"
#include "../core/sk_webview_resourceHandler/os/sk_webview_resourceHandler_response_windows.hxx"
#include "../core/utils/sk_string.hxx"

#include "cpp/modules/nodejs/fs/sk_ms_nodejs_fs.hxx"

BEGIN_SK_NAMESPACE

class SK_Module_System {
public:
	
	SK_Module_fs machine;
	SK_Module_fs application;
	SK_Module_fs fs;
	SK_Module_fs bdsfs;
	SK_Module_fs vfs;
	SK_Module_fs web;
	SK_Module_fs viewMngr;

	void performOperation(const SK_String& module, const SK_String& operation, const nlohmann::json& payload, SK_WebViewResource_Response& respondWith) {
		     if (module == "machine") machine.handleOperation(operation, payload, respondWith);
		else if (module == "application") application.handleOperation(operation, payload, respondWith);
		else if (module == "fs") fs.handleOperation(operation, payload, respondWith);
		else if (module == "bdsfs") bdsfs.handleOperation(operation, payload, respondWith);
		else if (module == "vfs") vfs.handleOperation(operation, payload, respondWith);
		else if (module == "web") web.handleOperation(operation, payload, respondWith);
		else if (module == "viewMngr") viewMngr.handleOperation(operation, payload, respondWith);
	};
};

END_SK_NAMESPACE
