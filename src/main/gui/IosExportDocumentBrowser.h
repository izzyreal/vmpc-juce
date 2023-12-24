#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <stdio.h>

namespace mpc { class Mpc; }

void doPresentShareOptions(void* nativeWindowHandle, mpc::Mpc*);

#endif
#endif
