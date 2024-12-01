#pragma once

#include <memory>
#include <fstream>
#include <string>

namespace mpc { class Mpc; }

class ImportDocumentUrlProcessor {
    
private:
    std::string destinationDir();
    
public:
  mpc::Mpc* mpc;
  bool destinationExists(const char* filename, const char* relativePath);
  std::shared_ptr<std::ostream> openOutputStream(const char* filename, const char* relativePath);
  void initFiles();
};
