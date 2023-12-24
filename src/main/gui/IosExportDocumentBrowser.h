#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <stdio.h>

#include <memory>
#include <fstream>

namespace mpc { class Mpc; }

class URLProcessor {
public:
  virtual bool destinationExists(const char* /* filename */, const char* /* relativeDir */) = 0;
  virtual std::shared_ptr<std::ostream> openOutputStream(const char* /* filename */, const char* /* relativeDir */) = 0;
  virtual void initFiles() = 0;
};
void doOpenIosDocumentBrowser(URLProcessor*, void* nativeWindowHandle);

#endif
#endif
