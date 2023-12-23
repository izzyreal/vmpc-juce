#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

void doPresentShareOptions(void* nativeWindowHandle);

#endif
#endif
