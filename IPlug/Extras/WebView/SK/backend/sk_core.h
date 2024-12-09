#pragma once

#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "wdlstring.h"
#include <functional>
#include <memory>

#include "sk_base.h"
#include "sk_ipc.h"

BEGIN_SK_NAMESPACE

class SK_Core
{
public:
    SK_IPC ipc;
private:
};

END_SK_NAMESPACE
